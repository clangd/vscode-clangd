// Automatically install clangd binary releases from GitHub.
//
// We don't bundle them with the package because they're big; we'd have to
// include all OS versions, and download them again with every extension update.
//
// There are several entry points:
//  - installation explicitly requested
//  - checking for updates (manual or automatic)
//  - no usable clangd found, try to recover
// These have different flows, but the same underlying mechanisms.
import AbortController from 'abort-controller';
import * as child_process from 'child_process';
import * as fs from 'fs';
import fetch from 'node-fetch';
import * as os from 'os';
import * as path from 'path';
import * as readdirp from 'readdirp';
import * as semver from 'semver';
import * as stream from 'stream';
import * as unzipper from 'unzipper';
import * as util from 'util';
import * as vscode from 'vscode';

export function activate(context: vscode.ExtensionContext) {
  context.subscriptions.push(vscode.commands.registerCommand(
      "clangd.install", async () => { installLatest(context); }));
  context.subscriptions.push(
      vscode.commands.registerCommand("clangd.update", async () => {
        const cur =
            vscode.workspace.getConfiguration('clangd').get<string>('path');
        checkUpdates(cur, true, context);
      }));
}

// The user has explicitly asked to install the latest clangd.
// Do so without further prompting, or report an error.
export async function installLatest(context: vscode.ExtensionContext) {
  const abort = new AbortController();
  try {
    const release = await Github.latestRelease();
    const asset = Github.chooseAsset(release);
    const clangdPath = await Install.install(release, asset, abort, context);
    await End.useClangd(`clangd ${release.name}`, clangdPath);
  } catch (e) {
    if (!abort.signal.aborted)
      End.installationFailed(e);
  }
}

// The extension has detected clangd isn't available.
// Inform the user, and if possible offer to install or adjust the path.
// Unlike installLatest(), we've had no explicit user request or consent yet.
export async function recover(context: vscode.ExtensionContext) {
  try {
    const release = await Github.latestRelease();
    const asset = Github.chooseAsset(release);
    const message =
        "The clangd language server was not found on your PATH.\n" +
        `Would you like to download and install clangd ${release.name}?`;
    if (await vscode.window.showInformationMessage(message, "Install")) {
      const abort = new AbortController();
      try {
        const clangdPath =
            await Install.install(release, asset, abort, context);
        await End.useClangd(`clangd ${release.name}`, clangdPath);
      } catch (e) {
        if (!abort.signal.aborted)
          End.installationFailed(e);
      }
    }
  } catch (e) {
    console.error("Auto-install failed: ", e);
    End.showInstallationHelp("The clangd language server is not installed.");
  }
}

// We have an apparently-valid clangd (`clangdPath`), check for updates.
export async function checkUpdates(clangdPath: string, requested: boolean,
                                   context: vscode.ExtensionContext) {
  // Gather all the version information to see if there's an upgrade.
  try {
    var release = await Github.latestRelease();
    var asset = Github.chooseAsset(release);
    var upgrade = await Version.upgrade(release, clangdPath);
  } catch (e) {
    console.log("Failed to check for clangd update: ", e);
    // We're not sure whether there's an upgrade: stay quiet unless asked.
    if (requested)
      vscode.window.showInformationMessage(
          "Failed to check for clangd update: " + e);
    return;
  }
  console.log("Checking for clangd update: available=", upgrade.new,
              " installed=", upgrade.old);
  // Bail out if the new version is better or comparable.
  if (upgrade == null) {
    if (requested)
      vscode.window.showInformationMessage("clangd is up-to-date");
    return;
  }
  // Ask to update. If this was unsolicited, let the user disable future checks.
  const message = "An updated clangd language server is available.\n " +
                  `Would you like to upgrade to clangd ${upgrade.new}? (from ${
                      upgrade.old})`;
  const install = `Install clangd ${upgrade.new}`;
  const dontCheck = "Don't ask again";
  const opts = requested ? [ install ] : [ install, dontCheck ];
  const response = await vscode.window.showInformationMessage(message, ...opts);
  if (response == install) {
    const abort = new AbortController();
    try {
      const clangdPath = await Install.install(release, asset, abort, context);
      await End.useClangd(`clangd ${release.name}`, clangdPath);
    } catch (e) {
      if (!abort.signal.aborted)
        End.installationFailed(e);
    }
  } else if (response == dontCheck) {
    vscode.workspace.getConfiguration('clangd').update(
        'checkUpdates', false, vscode.ConfigurationTarget.Global);
  }
}

// Bits for talking to github's release API
namespace Github {
export interface Release {
  name: string, tag_name: string, assets: Array<Asset>,
}
export interface Asset {
  name: string, browser_download_url: string,
}

// Fetch the metadata for the latest stable clangd release.
export async function latestRelease(): Promise<Release> {
  const url = "https://api.github.com/repos/clangd/clangd/releases/latest";
  const response = await fetch(url);
  if (!response.ok) {
    console.log(response.url, response.status, response.statusText);
    throw new Error("Can't fetch release: " + response.statusText);
  }
  return await response.json() as Release;
}

// Determine which release asset should be installed for this machine.
export function chooseAsset(release: Github.Release): Github.Asset|null {
  const variants: {[key: string]: string} = {
    "win32" : "windows",
    "linux" : "linux",
    "darwin" : "mac",
  };
  const variant = variants[os.platform()];
  // 32-bit vscode is still common on 64-bit windows, so don't reject that.
  if (variant && (os.arch() == "x64" || variant == "windows"))
    return release.assets.find(a => a.name.indexOf(variant) >= 0)
    throw new Error(
        `No clangd ${release.name} binary available for your platform`);
}
}

// Functions to finish an install workflow, informing the user.
namespace End {
// The configuration is not valid, the user should install clangd manually.
export async function showInstallationHelp(message: string) {
  const url = "https://clangd.llvm.org/installation.html";
  message += `\nSee ${url} for help.`
  if (await vscode.window.showErrorMessage(message, "Open website"))
  vscode.env.openExternal(vscode.Uri.parse(url));
}

// We tried to install clangd, but something went wrong.
export function installationFailed(e: any) {
  console.error("Failed to install clangd: ", e);
  const message = `Failed to install clangd language server: ${e}\n` +
                  "You may want to install it manually.";
  showInstallationHelp(message);
}

// Clangd was installed or detected, switch to that installation.
export async function useClangd(description: string, path: string) {
  vscode.workspace.getConfiguration('clangd').update(
      'path', path, vscode.ConfigurationTarget.Global);
  const message = `${description} enabled.\nReload the window to activate it.`;
  if (await vscode.window.showInformationMessage(message, 'Reload now') != null)
    vscode.commands.executeCommand('workbench.action.reloadWindow');
}
}

// Functions to download and install the releases, and manage the files on disk.
//
// File layout:
//  globalStoragePath/
//    install/
//      <version>/
//        clangd_<version>/            (outer director from zip file)
//          bin/clangd
//          lib/clang/...
//    download/
//      clangd-platform-<version>.zip  (deleted after extraction)
namespace Install {
// Download the binary archive `asset` from a github `release` and extract it
// to the extension's global storage location.
// The `abort` controller is signaled if the user cancels the installation.
// Returns the absolute path to the installed clangd executable.
export async function install(release: Github.Release, asset: Github.Asset,
                              abort: AbortController,
                              context: vscode.ExtensionContext):
    Promise<string> {
  const dirs = await createDirs(context);
  const extractRoot = path.join(dirs.install, release.tag_name);
  if (await util.promisify(fs.exists)(extractRoot)) {
    const message = `clangd ${release.name} is already installed!`;
    const use = "Use the installed version";
    const reinstall = "Delete it and reinstall";
    const response =
        await vscode.window.showInformationMessage(message, use, reinstall);
    if (response == use) {
      // Find clangd within the existing directory.
      let files = (await readdirp.promise(extractRoot)).map(e => e.fullPath);
      return findExecutable(files);
    } else if (response == reinstall) {
      // Remove the existing installation...
      await fs.promises.rmdir(extractRoot);
      // ... and continue with installation.
    } else {
      // User dismissed prompt, bail out.
      abort.abort();
      throw new Error(message);
    }
  }
  const zipFile = path.join(dirs.download, asset.name);
  await download(asset.browser_download_url, zipFile, abort);
  const archive = await unzipper.Open.file(zipFile);
  const executable = findExecutable(archive.files.map(f => f.path));
  await vscode.window.withProgress({
    location : vscode.ProgressLocation.Notification,
    title : "Extracting " + asset.name,
  },
                                   () => archive.extract({path : extractRoot}))
  const clangdPath = path.join(extractRoot, executable);
  await fs.promises.chmod(clangdPath, 0o755);
  await fs.promises.unlink(zipFile);
  return clangdPath;
}

// Create the 'install' and 'download' directories, and return absolute paths.
async function createDirs(context: vscode.ExtensionContext) {
  const install = path.join(context.globalStoragePath, "install");
  const download = path.join(context.globalStoragePath, "download");
  for (const dir of [install, download])
    await fs.promises.mkdir(dir, {'recursive' : true});
  return {install : install, download : download};
}

// Find the clangd executable within a set of files.
function findExecutable(paths: string[]): string {
  const filename = os.platform() == 'win32' ? 'clangd.exe' : 'clangd';
  const entry = paths.find(f => path.posix.basename(f) == filename ||
                                path.win32.basename(f) == filename);
  if (entry == null)
    throw new Error("Didn't find a clangd executable!");
  return entry;
}

// Downloads `url` to a local file `dest` (whose parent should exist).
// A progress dialog is shown, if it is cancelled then `abort` is signaled.
async function download(url: string, dest: string,
                        abort: AbortController): Promise<void> {
  console.log("Downloading ", url, " to ", dest);
  const progressOpts = {
    location : vscode.ProgressLocation.Notification,
    title : "Downloading " + path.basename(dest),
    cancellable : true
  };
  return vscode.window.withProgress(progressOpts, async (progress, cancel) => {
    const response = await fetch(url, {signal : abort.signal});
    if (!response.ok)
      throw new Error(`Failed to download $url`);
    cancel.onCancellationRequested((_) => abort.abort());
    const size = Number(response.headers.get('content-length'));
    let read = 0;
    let fraction = 0;
    response.body.on("data", (chunk: Buffer) => {
      read += chunk.length;
      const newFraction = read / size;
      progress.report({increment : 100 * (newFraction - fraction)});
      fraction = newFraction;
    });
    const out = fs.createWriteStream(dest);
    await util.promisify(stream.pipeline)(response.body, out);
  });
}
}

// Functions dealing with clangd versions.
//
// We parse both github release numbers and installed `clangd --version` output
// by treating them as SemVer ranges, and offer an upgrade if the version
// is unambiguously newer.
namespace Version {
export async function upgrade(release: Github.Release, clangdPath: string) {
  const releasedVer = released(release);
  const installedVer = await installed(clangdPath);
  if (!rangeGreater(releasedVer, installedVer))
    return null;
  return {old : installedVer.raw, new : releasedVer.raw};
}

// Get the version of an installed clangd binary using `clangd --version`.
async function installed(clangdPath: string): Promise<semver.Range> {
  const child = child_process.spawn(clangdPath, [ '--version' ],
                                    {stdio : [ 'ignore', 'pipe', 'ignore' ]});
  var output = '';
  for await (const chunk of child.stdout)
    output += chunk;
  console.log(clangdPath, " --version output: ", output);
  const prefix = "clangd version ";
  if (!output.startsWith(prefix))
    throw new Error(`Couldn't parse clangd --version output`);
  const rawVersion = output.substr(prefix.length).split(' ', 1)[0];
  return new semver.Range(rawVersion);
}

// Get the version of a github release, by parsing the tag or name.
function released(release: Github.Release): semver.Range {
  // Prefer the tag name, but fall back to the release name.
  return (!semver.validRange(release.tag_name) &&
          semver.validRange(release.name))
             ? new semver.Range(release.name)
             : new semver.Range(release.tag_name);
}

function rangeGreater(newVer: semver.Range, oldVer: semver.Range) {
  return semver.gtr(semver.minVersion(newVer), oldVer);
}
}
