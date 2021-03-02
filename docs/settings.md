# Settings

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
