import * as path from 'path';
import * as vscode from 'vscode';
import * as api from 'vscode-cmake-tools';
import * as vscodelc from 'vscode-languageclient/node';
import { exec } from 'child_process';

import {ClangdContext} from './clangd-context';

export function activate(context: ClangdContext) {
  const feature = new CMakeToolsFeature(context);
  context.client.registerFeature(feature);
}

export let clang_resource_dir: string | undefined;

namespace protocol {

export interface DidChangeConfigurationClientCapabilities {
  dynamicRegistration?: boolean;
}

export interface ClangdCompileCommand {
  // Directory
  workingDirectory: string;
  // Command line
  compilationCommand: string[];
}

export interface ConfigurationSettings {
  // File -> ClangdCompileCommand
  compilationDatabaseChanges: Object;
}

export interface DidChangeConfigurationParams {
  settings: ConfigurationSettings;
}

export namespace DidChangeConfigurationRequest {
export const type = new vscodelc.NotificationType<DidChangeConfigurationParams>(
    'workspace/didChangeConfiguration');
}

} // namespace protocol

class CMakeToolsFeature implements vscodelc.StaticFeature {
  private projectChange: vscode.Disposable = {dispose() {}};
  private codeModelChange: vscode.Disposable|undefined;
  private cmakeTools: api.CMakeToolsApi|undefined;
  private project: api.Project|undefined;
  private codeModel: Map<string, protocol.ClangdCompileCommand>|undefined;

  constructor(private readonly context: ClangdContext) {
    let cmakeTools = api.getCMakeToolsApi(api.Version.v1);
    if (cmakeTools === undefined)
      return;

    cmakeTools.then(api => {
      this.cmakeTools = api;
      if (this.cmakeTools === undefined)
        return;

      this.projectChange = this.cmakeTools.onActiveProjectChanged(
          this.onActiveProjectChanged, this);
      if (vscode.workspace.workspaceFolders !== undefined) {
        // FIXME: clangd not supported multi-workspace projects
        const projectUri = vscode.workspace.workspaceFolders[0].uri;
        this.onActiveProjectChanged(projectUri);
      }
    });
  }

  fillClientCapabilities(_capabilities: vscodelc.ClientCapabilities) {}
  fillInitializeParams(_params: vscodelc.InitializeParams) {}

  initialize(capabilities: vscodelc.ServerCapabilities,
             _documentSelector: vscodelc.DocumentSelector|undefined) {}
  getState(): vscodelc.FeatureState { return {kind: 'static'}; }
  dispose() {
    if (this.codeModelChange !== undefined)
      this.codeModelChange.dispose();
    this.projectChange.dispose();
  }

  async onActiveProjectChanged(path: vscode.Uri|undefined) {
    if (this.codeModelChange !== undefined) {
      this.codeModelChange.dispose();
      this.codeModelChange = undefined;
    }

    if (path === undefined)
      return;

    this.cmakeTools?.getProject(path).then(project => {
      this.project = project;
      this.codeModelChange =
          this.project?.onCodeModelChanged(this.onCodeModelChanged, this);
      this.onCodeModelChanged();
    });
  }

  async onCodeModelChanged() {
    const content = this.project?.codeModel;
    if (content === undefined)
      return;

    if (content.toolchains === undefined)
      return;

    const firstCompiler = content.toolchains.values().next().value as api.CodeModel.Toolchain || undefined;
    if (firstCompiler !== undefined) {
      let compilerName =
        firstCompiler.path.substring(firstCompiler.path.lastIndexOf(path.sep) + 1)
          .toLowerCase();
      if (compilerName.endsWith('.exe'))
        compilerName = compilerName.substring(0, compilerName.length - 4);
      if (compilerName.indexOf('clang') !== -1) {
        exec(`${firstCompiler.path} -print-file-name=`, (error, stdout, stderr) => {
          if (error) {
            return;
          }
          while (stdout.endsWith('\n') || stdout.endsWith('\r')) stdout = stdout.slice(0, -1);
          if (stdout !== clang_resource_dir) {
            clang_resource_dir = stdout;
            vscode.commands.executeCommand('clangd.restart');
          }
        });
      }
    }

    const request: protocol.DidChangeConfigurationParams = {
      settings: {compilationDatabaseChanges: {}}
    };

    let codeModelChanges: Map<string, protocol.ClangdCompileCommand> =
        new Map();
    content.configurations.forEach(configuration => {
      configuration.projects.forEach(project => {
        let sourceDirectory = project.sourceDirectory;
        project.targets.forEach(target => {
          if (target.sourceDirectory !== undefined)
            sourceDirectory = target.sourceDirectory;
          let commandLine: string[] = [];
          if (target.sysroot !== undefined)
            commandLine.push(`--sysroot=${target.sysroot}`);
          target.fileGroups?.forEach(fileGroup => {
            if (fileGroup.language === undefined)
              return;

            const compiler = content.toolchains?.get(fileGroup.language);
            if (compiler === undefined)
              return;

            commandLine.unshift(compiler.path);
            if (compiler.target !== undefined)
              commandLine.push(`--target=${compiler.target}`);

            let compilerName =
                compiler.path.substring(compiler.path.lastIndexOf(path.sep) + 1)
                    .toLowerCase();
            if (compilerName.endsWith('.exe'))
              compilerName = compilerName.substring(0, compilerName.length - 4);

            const ClangCLMode =
                compilerName === 'cl' || compilerName === 'clang-cl';
            const incFlag = ClangCLMode ? '/I' : '-I';
            const defFlag = ClangCLMode ? '/D' : '-D';

            fileGroup.compileCommandFragments?.forEach(commands => {
              commands.split(/\s/g).forEach(
                  command => { commandLine.push(command); });
            });
            fileGroup.includePath?.forEach(
                include => { commandLine.push(`${incFlag}${include.path}`); });
            fileGroup.defines?.forEach(
                define => { commandLine.push(`${defFlag}${define}`); });
            fileGroup.sources.forEach(source => {
              const file = sourceDirectory.length != 0
                               ? sourceDirectory + path.sep + source
                               : source;
              const command: protocol.ClangdCompileCommand = {
                workingDirectory: sourceDirectory,
                compilationCommand: commandLine
              };
              codeModelChanges.set(file, command);
            });
          });
        });
      });
    });

    const codeModel = new Map(codeModelChanges);
    this.codeModel?.forEach((cc, file) => {
      if (!codeModelChanges.has(file)) {
        const command: protocol.ClangdCompileCommand = {
          workingDirectory: '',
          compilationCommand: []
        };
        codeModelChanges.set(file, command);
        return;
      }
      const command = codeModelChanges.get(file);
      if (command?.workingDirectory === cc.workingDirectory &&
          command?.compilationCommand.length === cc.compilationCommand.length &&
          command?.compilationCommand.every(
              (val, index) => val === cc.compilationCommand[index])) {
        codeModelChanges.delete(file);
      }
    });
    this.codeModel = codeModel;

    if (codeModelChanges.size === 0)
      return;

    codeModelChanges.forEach(
        (cc, file) => {Object.assign(
            request.settings.compilationDatabaseChanges, {[file]: cc})});

    this.context.client.sendNotification(
        protocol.DidChangeConfigurationRequest.type, request);
  }
}
