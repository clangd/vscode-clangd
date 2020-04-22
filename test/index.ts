// Entry point for all tests.
// Spawns VSCode with our extension, and then runs *.test.ts in that context.

import * as glob from 'glob';
import * as Mocha from 'mocha';
import * as path from 'path';
import {runTests} from 'vscode-test';

// The entry point under VSCode - find the test files and run them in Mocha.
export function run(): Promise<void> {
  const mocha = new Mocha({ui : 'tdd', color: true});
  const testsRoot = path.resolve(__dirname, '..');

  return new Promise((c, e) => {
    glob('**/**.test.js', {cwd : testsRoot}, (err, files) => {
      if (err) {
        return e(err);
      }
      files.forEach(f => mocha.addFile(path.resolve(testsRoot, f)));

      try {
        // Run the mocha test
        mocha.run(failures => {
          if (failures > 0) {
            e(new Error(`${failures} tests failed.`));
          } else {
            c();
          }
        });
      } catch (err) {
        e(err);
      }
    });
  });
}

// The main entry point: its job is to launch VSCode and execute run() under it.
async function main() {
  try {
    // The extension to be loaded is in the parent directory.
    const extensionDevelopmentPath = path.resolve(__dirname, '../');
    // The run() function to run in vscode is defined in this file.
    const extensionTestsPath = __filename;
    // Download VS Code, unzip it and run the integration test
    await runTests({extensionDevelopmentPath, extensionTestsPath});
  } catch (err) {
    console.error('Failed to run tests');
    process.exit(1);
  }
}

if (require.main === module) {
  main();
}

