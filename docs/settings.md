# Settings

## Multi-Root Workspaces

### `clangd.enablePerFolderServer`

Controls whether to run a separate clangd instance for each workspace folder.

- **Default**: `false`
- **Scope**: `window` (workspace-level setting)

When `false` (default), a single clangd instance handles all workspace
folders. This is simpler and uses less memory.

When `true`, each project gets its own clangd instance, which starts when one
of the project's files is shown in an editor. Its working directory is the
project's folder, and `${workspaceFolder}` in `clangd.arguments` and
`clangd.fallbackFlags` refers to it. Files outside all projects are not
served. Each instance writes its own trace: with `clangd.trace` set to
`/tmp/trace.json`, the instance of the project `app` writes
`/tmp/trace.app-<hash>.json`, where `<hash>` identifies the project's root.
Changing the setting restarts clangd.

```json
{
  "clangd.enablePerFolderServer": true
}
```

### Per-folder settings

The following settings have `resource` scope. When
`clangd.enablePerFolderServer` is `true`, each clangd instance reads them from
the settings of its workspace folder, so they can differ between folders:

- `clangd.arguments`
- `clangd.fallbackFlags`
- `clangd.trace`
- `clangd.restartAfterCrash`
- `clangd.onConfigChanged`
- `clangd.onConfigChangedForceEnable`
- `clangd.enableCodeCompletion`
- `clangd.enableHover`
- `clangd.serverCompletionRanking`
- `clangd.inactiveRegions.useBackgroundHighlight`
- `clangd.inactiveRegions.opacity`

When `clangd.enablePerFolderServer` is `false`, a multi-root workspace has a
single clangd instance, and folder values of these settings are ignored in
favor of the workspace and user values.

Settings that concern the clangd executable or the extension as a whole, such
as `clangd.path`, `clangd.useScriptAsExecutable`, `clangd.checkUpdates` and
`clangd.enable`, are shared by all instances.

## Security

This extension allows configuring command-line flags to the `clangd` executable.

Being able to configure these per-workspace can be convenient, but runs the risk
of an attacker changing your flags if you happen to open their repository.
Similarly, allowing the workspace to control the `clangd.path` setting is risky.

When these flags are configured in the workspace, the extension will prompt you
whether to use them or not. If you're unsure, click "No".

![Screenshot: Workspace trying to override clangd.path](workspace-override.png)

If you choose "Yes" or "No", the answer will be remembered for this workspace.
(If the value changes, you will be prompted again).
