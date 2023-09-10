import * as vscode from 'vscode';

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
