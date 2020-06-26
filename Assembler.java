import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

class Assembler {
    final int HEADER_BYTES = 4;
    final int INITIALIZATION_BYTES = 20;

    Map<String, InstructionInfo> getMnemonicToInsructionInfo() {
        final Map<String, InstructionInfo> instructionsMap = new HashMap<>();

        instructionsMap.put("bipush", new InstructionInfo(true, false, (byte) 0x19, 2));
        instructionsMap.put("dup", new InstructionInfo(false, false, (byte) 0x0E, 1));
        instructionsMap.put("goto", new InstructionInfo(true, true, (byte) 0x3C, 3));
        instructionsMap.put("iadd", new InstructionInfo(false, false, (byte) 0x02, 1));
        instructionsMap.put("iand", new InstructionInfo(false, false, (byte) 0x08, 1));
        instructionsMap.put("ifeq", new InstructionInfo(true, true, (byte) 0x47, 3));
        instructionsMap.put("iflt", new InstructionInfo(true, true, (byte) 0x43, 3));
        instructionsMap.put("if_icmpeq", new InstructionInfo(true, true, (byte) 0x4B, 3));
        instructionsMap.put("iload", new InstructionInfo(true, false, (byte) 0x1C, 2));
        instructionsMap.put("invokevirtual", new InstructionInfo(true, false, (byte) 0x55, 3));
        instructionsMap.put("ior", new InstructionInfo(false, false, (byte) 0x0B, 1));
        instructionsMap.put("ireturn", new InstructionInfo(false, false, (byte) 0x6B, 1));
        instructionsMap.put("istore", new InstructionInfo(true, false, (byte) 0x22, 2));
        instructionsMap.put("isub", new InstructionInfo(false, false, (byte) 0x05, 1));
        instructionsMap.put("ldc_w", new InstructionInfo(true, false, (byte) 0x32, 3));
        instructionsMap.put("nop", new InstructionInfo(false, false, (byte) 0x01, 1));
        instructionsMap.put("pop", new InstructionInfo(false, false, (byte) 0x10, 1));
        instructionsMap.put("swap", new InstructionInfo(false, false, (byte) 0x13, 1));
        instructionsMap.put("wide", new InstructionInfo(false, false, (byte) 0x28, 1));


        // this instruction has 2 operands instead of one operand, like other instructions.
        instructionsMap.put("iinc", new InstructionInfo(true, false, (byte) 0x36, 3));

        return instructionsMap;
    }

    // helper to check if the string is a number or a identifier.
    boolean isNumeric(final String possibleNumber) {
        try {
            Double.parseDouble(possibleNumber);
        } catch (NumberFormatException nfe) {
            return false;
        }
        return true;
    }

    byte[] assemble(final List<String> fileLines) {
        int variablesCounter = 0;
        final Map<String, Integer> variableToVarindex = new HashMap<>();
        final Map<String, Integer> labelToLineNumber = new HashMap<>();
        final Map<String, InstructionInfo> mnemonics = getMnemonicToInsructionInfo();

        // two steps assembler: first, locate labels and variables

        // byte 0 is skipped, so we start from byte 1
        // first step: locate possible labels and save byte location
        int currentBytes = 1;
        for (int i = 0; i < fileLines.size(); i++) {
            final String currentLine = fileLines.get(i);

            // split lines by whitespace
            final String[] components = currentLine.trim().split("\\s+");

            int mnemonicPosition = 0;
            // check if first element is label or direct instruction
            // if it isn't a direct instruction, then we assume it is a label.
            if (!mnemonics.containsKey(components[0])) {
                labelToLineNumber.put(components[0], currentBytes);

                // mnemonic expected to be on position 1, because pos. 0 is the label
                mnemonicPosition = 1;
            }

            final InstructionInfo instructionInfo = mnemonics.get(components[mnemonicPosition]);
            currentBytes += instructionInfo.size;
        }

        // header: 4 bytes, 20 initialization bytes, and currentBytes - 1 to contain program.
        // - 1 because current bytes also counts the first byte (which will be at position 1024, and skipped)
        // so we're going to write only the currentBytes - 1 bytes to the file and write it from position 1025.
        final int expectedProgramSize = HEADER_BYTES + INITIALIZATION_BYTES + currentBytes - 1;

        // second step: now locate variables, we find labels first so we don't assume a label name is a new variable.
        for (int i = 0; i < fileLines.size(); i++) {
            final String currentLine = fileLines.get(i);

            // split lines by whitespace
            final String[] components = currentLine.trim().split("\\s+");

            int instructionStart = 0;

            // check if first element is label or direct instruction
            // if it isn't a direct instruction, then we assume it is a label.
            if (labelToLineNumber.containsKey(components[instructionStart])) {
                instructionStart++;
            }

            // next should be instruction;
            final String currentInstruction = components[instructionStart];
            final InstructionInfo instructionInfo = mnemonics.get(currentInstruction);

            if (instructionInfo == null) {
                throw new Error("couldn't decode instruction: " + currentInstruction + " on line " + i);
            }

            if (instructionInfo.hasOperands) {
                // if we have operands, let's check for possible variables;
                instructionStart++;
                for (int j = instructionStart; j < components.length; j++) {
                    final String currentComponent = components[j];
                    if (!isNumeric(currentComponent) && !labelToLineNumber.containsKey(currentComponent)
                            && !variableToVarindex.containsKey(currentComponent)) {
                        // must be a new variable identifier.
                        variableToVarindex.put(currentComponent, variablesCounter);
                        variablesCounter += 1;
                    }
                }
            }
        }

        final byte[] programBytes = new byte[expectedProgramSize];
        // write header
        programBytes[3] = (byte) (expectedProgramSize >> 24);
        programBytes[2] = (byte) (expectedProgramSize >> 16);
        programBytes[1] = (byte) (expectedProgramSize >> 8);
        programBytes[0] = (byte) expectedProgramSize;

        // write initialization bytes, positions [4, 23]
        final int LV_ADDRESS = 0x1001;
        final int SP_ADDRESS = LV_ADDRESS + variablesCounter;

        int currentPosition = HEADER_BYTES;
        programBytes[currentPosition] = 0x00;
        programBytes[currentPosition + 1] = 0x73; // init
        programBytes[currentPosition + 2] = 0x00;
        programBytes[currentPosition + 3] = 0x00;
        programBytes[currentPosition + 4] = 0x06; // cpp = 6
        programBytes[currentPosition + 5] = 0x00;
        programBytes[currentPosition + 6] = 0x00;
        programBytes[currentPosition + 7] = 0x00;
        programBytes[currentPosition + 8] = (byte) LV_ADDRESS;        // 1o byte de lv
        programBytes[currentPosition + 9] = (byte) (LV_ADDRESS >> 8);   // 2o byte de lv
        programBytes[currentPosition + 10] = (byte) (LV_ADDRESS >> 16); // 3o byte de lv
        programBytes[currentPosition + 11] = (byte) (LV_ADDRESS >> 24); // 4o byte de lv
        programBytes[currentPosition + 12] = 0x00; // pc
        programBytes[currentPosition + 13] = 0x04; // pc = 0x0400
        programBytes[currentPosition + 14] = 0x00; // pc
        programBytes[currentPosition + 15] = 0x00; // pc
        programBytes[currentPosition + 16] = (byte) SP_ADDRESS;
        programBytes[currentPosition + 17] = (byte) (SP_ADDRESS >> 8);
        programBytes[currentPosition + 18] = (byte) (SP_ADDRESS >> 16);
        programBytes[currentPosition + 19] = (byte) (SP_ADDRESS >> 24);

        currentPosition += INITIALIZATION_BYTES;

        currentBytes = 1;
        for (final String line : fileLines) {
            final String[] components = line.trim().split("\\s+");

            int instructionStart = 0;

            // jump label
            if (labelToLineNumber.containsKey(components[instructionStart])) {
                instructionStart++;
            }

            // get instruction;
            final String currentInstruction = components[instructionStart];
            final InstructionInfo instructionInfo = mnemonics.get(currentInstruction);

            if (instructionInfo == null) {
                throw new Error("couldn't decode instruction: " + currentInstruction);
            }

            programBytes[currentPosition] = instructionInfo.code;
            currentPosition += 1;

            // write operands, if available
            instructionStart++;
            if (instructionInfo.hasOperands) {
                // here we can have labels, variables or values;
                // only jumps have labels, so we can verify that first.
                final String currentValue = components[instructionStart];

                if (instructionInfo.isJump) {
                    // should be label to jump
                    final int destinationByte = labelToLineNumber.get(currentValue);
                    final int offset = destinationByte - currentBytes;

                    // write offset
                    // PS!!!!!!!! Jump operations are being treated as big endian, so we write as big endian!!!
                    // write 2 bytes
                    programBytes[currentPosition] = (byte) (offset >> 8);
                    programBytes[currentPosition + 1] = (byte) offset;
                    currentPosition += 2;
                } else {
                    // so it's a variable or a value. we can lookup if it's a variable or write the direct value
                    if (isNumeric(currentValue)) {
                        // just write value
                        final int dataToWrite = Integer.parseInt(currentValue);
                        programBytes[currentPosition] = (byte) dataToWrite;
                    } else {
                        // write variable index
                        final int varIndex = variableToVarindex.get(currentValue);
                        programBytes[currentPosition] = (byte) varIndex;
                    }
                    currentPosition++;
                }
            }
            currentBytes += instructionInfo.size;
        }

        return programBytes;
    }

    public static void main(String[] args) throws IOException {
        final String fileName = args[0];
        final String outputName = args[1];
        final Path filePath = Paths.get(fileName);
        final Path outputPath = Paths.get(outputName);
        final List<String> lines = Files.readAllLines(filePath);

        final byte[] program = new Assembler().assemble(lines);
        Files.write(outputPath, program);
    }

    class InstructionInfo {
        boolean hasOperands;
        boolean isJump;
        byte code;
        int size;

        /**
         * @param hasOperands Indicates if the instruction has operands
         * @param isJump Indicates if the instruction is a jump. Jumps need to be encoded a bit different than
         *               other instructions.
         * @param code The binary code (opcode) for this instruction
         * @param size The instruction size in bytes (opcode + operands, if any)
         */
        public InstructionInfo(boolean hasOperands, boolean isJump, byte code, int size) {
            this.hasOperands = hasOperands;
            this.isJump = isJump;
            this.code = code;
            this.size = size;
        }
    }
}