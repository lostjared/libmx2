# Command-Line Options
    -c <input_file>

Use this option to specify the file you want to compress.

    -z <input_file>

Use this option to specify the file you want to decompress.

    -o <output_file>

Use this option to specify the name of the output file.
Note: When compressing, the output file must end with the extension .mxmod.z.

    -h or -v

These options display help or version information.

# Usage Guidelines
# Compression Mode:


Command:
        
        mxmod_compress -c path/to/input_file.mxmod -o path/to/output_file.mxmod.z

Description:
Reads the specified input file, compresses its content using zlib, and writes the compressed data to the output file.
Make sure the output file’s name ends with .mxmod.z so that the program recognizes it as a valid model file for compression.
Decompression Mode:

Command TO Decompress:

    mxmod_compress -z path/to/input_file.mxmod.z -o path/to/output_file.mxmod

Description:
Reads the specified compressed input file, decompresses it, and writes the decompressed content to the output file.
Important:

You cannot specify both compression (-c) and decompression (-z) in the same command. The program will exit with an error if both are provided.
Ensure that you always provide an output file with the -o option.
