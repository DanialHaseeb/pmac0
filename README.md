# Parallelised Message Authentication Code 0 (PMAC0)

This is an implementation of PMAC (Parallelizable Message Authentication Code) using MPI (Message Passing Interface) for distributed computation. PMAC is a cryptographic primitive used to provide integrity and authentication for data. Confidentiality is _not_ a primary concern here.

## Prerequisites

You need to have an MPI library (such as OpenMPI) installed on your machine to compile and run this code.

## Usage

To serialize 
```sh
gcc serialize.c -o target/serialize
./target/serialize input.txt output/serialized_output.txt
```

Compile the program using mpicc (or your equivalent MPI compiler), like so:

```sh
mpicc pmac0.c -o pmac0
```

Then, run it with mpirun (or your equivalent command to start an MPI program), like so:

```sh
mpirun -np 4 pmac0 <sign|verify> <f1f2(otp|vigenere|rc4)> <filepath> [<tagfilepath>]
```

Here, "-np 4" indicates that we want to run the program with 4 processes. You can adjust this to your preference.

The first argument after the program name must be either "sign" or "verify":

- In "sign" mode, you provide the path to a file as input, and the program will compute the PMAC tag for this file and write it to a new file in the same directory.

  For example:

  ```sh
  mpirun -np 4 pmac0 sign otp input.txt
  ```

  This will create a file named `input.txt.tag` in the same directory as `input.txt`, containing the computed PMAC tag.

- In "verify" mode, you provide the path to a file and the path to a file containing a previously computed PMAC tag, and the program will compute the PMAC tag for the file again and compare it to the provided tag.

  For example:

  ```sh
  mpirun -np 4 pmac0 verify otp input.txt input.txt.tag
  ```

  The program will output either "Tag verification succeeded" if the computed tag matches the provided tag, or "Tag verification failed" otherwise.

## Note

This is a basic implementation of PMAC0 and is not suitable for handling sensitive data or use in a production environment.

In particular, the secret keys used for the PMAC0 computations are hardcoded and not secret at all, and the functions F1 and F2, which in a real PMAC implementation would be cryptographic pseudorandom functions, are here can be simple one-time pads.

Moreover, this program does not handle errors robustly and is not optimised for performance. Its purpose is to illustrate how PMAC0 and MPI can be used together to compute a MAC in parallel.
