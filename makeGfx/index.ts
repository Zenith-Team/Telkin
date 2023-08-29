import { readFileSync, writeFileSync } from "fs";
import crlf from "crlf-normalize";



const args: string[] = process.argv.slice(2);
let file: string = readFileSync(args[0]!, "utf8");

// remove comments (they start with '#')
file = file.replace(/#.*/g, "");

// remove "... .type" and "... .size" lines
file = file.replace(/.*\.type.*/g, "");
file = file.replace(/.*\.size.*/g, "");

// remove "... .align" lines
file = file.replace(/.*\.align.*/g, "");

// remove "... .gh ..." lines
file = file.replace(/.*\.gh.*/g, "");

// remove "... .global ..." lines
file = file.replace(/.*\.global.*/g, "");

// remove "... .text" lines
file = file.replace(/.*\.text.*/g, "");

// remove "... .data" lines
file = file.replace(/.*\.data.*/g, "");

// remove "... .section..." lines
file = file.replace(/.*\.section.*/g, "");

// replace labels that are like ".L123" with "label_123"
file = file.replace(/\.L(\d+)/g, "label_$1");

// replace "%(hiadj)(x-1)" with "%(hiadj)(x)"
file = file.replace(/%(hiadj)\((.*)-1\)/g, "%$1($2)");

// replace "%(hiadj)(x)" with x@ha
file = file.replace(/%(hiadj)\((.*)\)/g, "$2@ha");

// replace "%(lo)(x-1)" with "%(lo)(x)"
file = file.replace(/%(lo)\((.*)-1\)/g, "$1($2)");

// replace "%(lo)(x)" with x@l
file = file.replace(/%(lo)\((.*)\)\((.*)\)/g, "$2@l($3)");

// replace uses of sp with r1 (but make sure to not replace "sp" in the middle of a word)
file = file.replace(/([^a-zA-Z0-9_])sp([^a-zA-Z0-9_])/g, "$1r1$2");

// addi	r3, r24, %lo(logMsg) -> addi	r3, r24, logMsg@l
file = file.replace(/(addi\s+r\d+,\s+r\d+,\s+)(%lo\()(.*)\)/g, "$1$3@l");

// ".comm foo,4,4" -> "foo: .space 4"
file = file.replace(/\.comm\s+(\w+),(\d+),(\d+)/g, (match, p1, p2, p3) => {
    return `${p1}:.space ${p2}`;
});

// ".space 5" -> ".byte 0, 0, 0, 0, 0"
file = file.replace(/\.space\s+(\d+)/g, (match, p1) => {
    let str = ".byte ";
    for (let i = 0; i < parseInt(p1); i++) {
        str += "0, ";
    }
    return str.slice(0, -2);
});

// replace : with :\n
file = file.replace(/:/g, ":\n");

// unsupported asm instructions
file = file.replace("subic.	r6, r6, 1", ".byte 0x34, 0xC6, 0xFF, 0xFF");
file = file.replace("subic.	r9, r9, 1", ".byte 0x35, 0x29, 0xFF, 0xFF");
file = file.replace("subic.	r8, r8, 1", ".byte 0x35, 0x08, 0xFF, 0xFF");
file = file.replace("subic.	r7, r7, 1", ".byte 0x34, 0xE7, 0xFF, 0xFF");

// remove empty lines
file = file.replace(/^\s*[\r\n]/gm, "");

// remove indents and 4 spaces
file = file.replace(/^\t+/gm, "");
file = file.replace(/^    +/gm, "");

interface TargetType {
    Game: string;
    Region: "US" | "EU" | "JP" | string;
    RPXHash: string; // these should be numbers - ex "0x12345678"
    AcquireInstruction: string;
    FindExportInstruction: string;
    InjectAddr: string;
}
function TelkinDefineRegion(file: string, makeArgs: TargetType): string {
	const header =
`[${makeArgs.Game}_TLoader_${makeArgs.Region}] ; Telkin Loader - Built on ${new Date().toLocaleString()}
moduleMatches = ${makeArgs.RPXHash}

BLOSDynLoad_Acquire = ${makeArgs.AcquireInstruction}
BOSDynLoad_FindExport = ${makeArgs.FindExportInstruction}

.origin = codecave`;

	const footer = `${makeArgs.InjectAddr} = b __tloader_init`;

	return header + "\r\n" + file + "\r\n" + footer;
}

let outFile: string = "";

const targets: TargetType[] = (await import("./targets.json", { assert: { type: "json" }})).default.targets;

for (const target of targets) {
    outFile += TelkinDefineRegion(file, target) + "\r\n\r\n";
}

// convert to CRLF
outFile = crlf(outFile);
writeFileSync(args[1]!, outFile, "utf8");
