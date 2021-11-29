import { ASTNode } from './ast';
export { ASTNode } from './ast';

// Support for exported apis
export interface API {
    ast(): Promise<ASTNode | null>
}