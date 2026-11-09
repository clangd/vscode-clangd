import * as sinon from 'sinon';
import * as vscode from 'vscode';

import * as clangdContext from '../src/clangd-context';
import {ClangdContextManager} from '../src/clangd-context-manager';
import * as install from '../src/install';
import {ClangdProject, uriContains} from '../src/project';

export class MockTextDocument implements vscode.TextDocument {
  readonly uri: vscode.Uri;
  readonly fileName: string;
  readonly isUntitled: boolean = false;
  readonly languageId: string;
  readonly version: number = 0;
  readonly isDirty: boolean = false;
  readonly isClosed: boolean = false;

  constructor(uri: vscode.Uri, languageId?: string) {
    this.uri = uri;
    this.fileName = this.uri.fsPath;
    this.languageId = languageId ?? 'plaintext';
  }

  save(): Thenable<boolean> { throw new Error('Method not implemented.'); }

  readonly eol: vscode.EndOfLine = vscode.EndOfLine.LF;

  get lineCount(): number { throw new Error('Method not implemented.'); }

  lineAt(line: number): vscode.TextLine;
  lineAt(position: vscode.Position): vscode.TextLine;
  lineAt(position: number|vscode.Position): vscode.TextLine {
    throw new Error('Method not implemented.');
  }

  offsetAt(position: vscode.Position): number {
    throw new Error('Method not implemented.');
  }

  positionAt(offset: number): vscode.Position {
    throw new Error('Method not implemented.');
  }

  getText(range?: vscode.Range): string {
    throw new Error('Method not implemented.');
  }

  getWordRangeAtPosition(position: vscode.Position,
                         regex?: RegExp): vscode.Range|undefined {
    throw new Error('Method not implemented.');
  }

  validateRange(range: vscode.Range): vscode.Range {
    throw new Error('Method not implemented.');
  }

  validatePosition(position: vscode.Position): vscode.Position {
    throw new Error('Method not implemented.');
  }
}

export class MockTextEditor implements vscode.TextEditor {
  readonly document: vscode.TextDocument;
  selection: vscode.Selection = new vscode.Selection(0, 0, 0, 0);
  selections: readonly vscode.Selection[] = [];
  readonly visibleRanges: readonly vscode.Range[] = [];
  options: vscode.TextEditorOptions = {};
  readonly viewColumn: vscode.ViewColumn|undefined;

  constructor(document: vscode.TextDocument) { this.document = document; }

  edit(callback: (editBuilder: vscode.TextEditorEdit) => void, options
       ?: {readonly undoStopBefore: boolean; readonly undoStopAfter: boolean}):
      Thenable<boolean> {
    throw new Error('Method not implemented.');
  }

  insertSnippet(snippet: vscode.SnippetString,
                location?: vscode.Position|vscode.Range|
                readonly vscode.Position[]|readonly vscode.Range[],
                options?: {
                  readonly undoStopBefore: boolean; readonly undoStopAfter:
                                                                 boolean
                }): Thenable<boolean> {
    throw new Error('Method not implemented.');
  }

  setDecorations(decorationType: vscode.TextEditorDecorationType,
                 rangesOrOptions: readonly vscode.Range[]|
                 readonly vscode.DecorationOptions[]): void {
    throw new Error('Method not implemented.');
  }

  revealRange(range: vscode.Range,
              revealType?: vscode.TextEditorRevealType): void {
    throw new Error('Method not implemented.');
  }

  show(column?: vscode.ViewColumn): void {
    throw new Error('Method not implemented.');
  }

  hide(): void { throw new Error('Method not implemented.'); }
}

// Stands in for a ClangdContext; only what the manager uses.
export class FakeContext {
  subscriptions: vscode.Disposable[] = [];
  private readonly stateEmitter = new vscode.EventEmitter<void>();
  client = {onDidChangeState: this.stateEmitter.event};
  starting = false;
  started = false;
  disposed = false;

  constructor(readonly project: ClangdProject|undefined) {}

  async startClient() {
    this.started = true;
    this.stateEmitter.fire();
  }
  servesDocument(document: vscode.TextDocument) {
    return !this.project || uriContains(this.project.uri, document.uri);
  }
  clientIsStarting() { return this.starting; }
  clientIsRunning() { return this.started && !this.starting && !this.disposed; }
  clientIsStartingOrRunning() {
    return this.clientIsStarting() || this.clientIsRunning();
  }
  async clientSettled() {}
  dispose() {
    this.subscriptions.forEach((d) => { d.dispose(); });
    this.disposed = true;
  }
}

// Resolves once pending promise callbacks have run.
export function settle() {
  return new Promise(resolve => setTimeout(resolve, 0));
}

export const cppEditor = (path: string) =>
    new MockTextEditor(new MockTextDocument(vscode.Uri.file(path), 'cpp'));

// A ClangdContextManager whose contexts are FakeContexts, in a workspace and
// window controlled by the test. The commands and views are not registered:
// they would collide with the ones of the extension under test.
export class ManagerHarness implements vscode.Disposable {
  multiProject = true;
  folders: vscode.WorkspaceFolder[] = [];
  visibleEditors: vscode.TextEditor[] = [];
  activeEditor: vscode.TextEditor|undefined;
  // The contexts in the order of their creation.
  readonly contexts: FakeContext[] = [];
  readonly channels:
      {name: string, info: sinon.SinonSpy, dispose: sinon.SinonSpy}[] = [];
  // ClangdContext.create() and install.activate().
  readonly create: sinon.SinonStub;
  readonly activate: sinon.SinonStub;
  readonly manager: ClangdContextManager;

  private readonly sandbox = sinon.createSandbox();
  private readonly foldersChanged =
      new vscode.EventEmitter<vscode.WorkspaceFoldersChangeEvent>();
  private readonly visibleEditorsChanged =
      new vscode.EventEmitter<readonly vscode.TextEditor[]>();
  private readonly activeEditorChanged =
      new vscode.EventEmitter<vscode.TextEditor|undefined>();

  constructor() {
    const sandbox = this.sandbox;
    sandbox.stub(vscode.workspace, 'getConfiguration').returns({
      get: (key: string) =>
          key === 'enablePerFolderServer' ? this.multiProject : undefined
    } as unknown as vscode.WorkspaceConfiguration);
    sandbox.stub(vscode.workspace, 'workspaceFolders').get(() => this.folders);
    sandbox.stub(vscode.workspace, 'onDidChangeWorkspaceFolders')
        .callsFake(this.foldersChanged.event);
    sandbox.stub(vscode.window, 'visibleTextEditors')
        .get(() => this.visibleEditors);
    sandbox.stub(vscode.window, 'onDidChangeVisibleTextEditors')
        .callsFake(this.visibleEditorsChanged.event);
    sandbox.stub(vscode.window, 'activeTextEditor')
        .get(() => this.activeEditor);
    sandbox.stub(vscode.window, 'onDidChangeActiveTextEditor')
        .callsFake(this.activeEditorChanged.event);
    sandbox.stub(vscode.window, 'createOutputChannel')
        .callsFake(
            ((name: string) => {
              const channel = {name, info: sinon.spy(), dispose: sinon.spy()};
              this.channels.push(channel);
              return channel;
            }) as any);
    sandbox.stub(clangdContext, 'registerFeatureCommands').returns([]);
    this.activate =
        sandbox.stub(install, 'activate').resolves('/usr/bin/clangd');
    this.create =
        sandbox.stub(clangdContext.ClangdContext, 'create')
            .callsFake((async (_path: string, _channel: unknown,
                               _manager: unknown, project?: ClangdProject) =>
                            this.newContext(project)) as any);
    this.manager = new ClangdContextManager(
        '/tmp', {info() {}, error() {}} as unknown as vscode.LogOutputChannel);
  }

  newContext(project: ClangdProject|undefined): FakeContext {
    const context = new FakeContext(project);
    this.contexts.push(context);
    return context;
  }

  // Makes ClangdContext.create() wait until the returned function is called.
  blockCreation(): () => void {
    let resolve!: () => void;
    const created = new Promise<void>(r => resolve = r);
    this.create.callsFake(async (_path: string, _channel: unknown,
                                 _manager: unknown,
                                 project?: ClangdProject) => {
      await created;
      return this.newContext(project);
    });
    return resolve;
  }

  // Makes the first install.activate() wait until the returned function is
  // called.
  blockActivation(): () => void {
    let resolve!: (path: string) => void;
    this.activate.onFirstCall().callsFake(() => new Promise(r => resolve = r));
    return () => resolve('/usr/bin/clangd');
  }

  // Sets the workspace folders; their names default to the basenames.
  setFolders(paths: string[], names: string[] = []) {
    this.folders = paths.map((path, index) => ({
                               uri: vscode.Uri.file(path),
                               name: names[index] ?? path.split('/').pop()!,
                               index
                             }));
    this.foldersChanged.fire({added: [], removed: []});
  }

  setVisibleEditors(editors: vscode.TextEditor[]) {
    this.visibleEditors = editors;
    this.visibleEditorsChanged.fire(editors);
  }

  setActiveEditor(editor: vscode.TextEditor|undefined) {
    this.activeEditor = editor;
    this.activeEditorChanged.fire(editor);
  }

  dispose() {
    this.manager.dispose();
    this.sandbox.restore();
  }
}
