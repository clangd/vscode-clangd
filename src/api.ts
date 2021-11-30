import * as vscodelc from 'vscode-languageclient/node';

type MarkupKind = 'PlainText' | 'Markdown';

export interface MarkupContent {
    kind: MarkupKind;
    value: string;
}

export interface Hover {
    contents: MarkupContent
    range?: vscodelc.Range
}

// Support for exported apis
export interface API {
    hover(): Promise<Hover|null>
}