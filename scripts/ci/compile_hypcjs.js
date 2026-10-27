// Standard JSON adapter used by the cross-platform bytecode comparison: compiles each line of stdin
// as one input and writes each output as one line.
'use strict';
const fs = require('fs');
const path = require('path');
const compiler = require(path.resolve(process.argv[2]));
for (const input of fs.readFileSync(0, 'utf8').split('\n').filter(Boolean))
    process.stdout.write(JSON.stringify(JSON.parse(compiler.compile(input))) + '\n');
