# Parallelizable Message Authentication Code (PMAC)

This is an implementation of PMAC (Parallelizable Message Authentication Code) using MPI (Message Passing Interface) for distributed computation. PMAC is a cryptographic primitive used to provide integrity and authentication for data.

## Prerequisites

You need to have an MPI library (such as OpenMPI) installed on your machine to compile and run this code.

## Usage

Compile the program using mpicc (or your equivalent MPI compiler), like so:

```sh
mpicc pmac.c -o pmac
```

Then, run it with mpirun (or your equivalent command to start an MPI program), like so:

```sh
mpirun -np 4 pmac <sign|verify> <filepath> [<tagfilepath>]
```

Here, "-np 4" indicates that we want to run the program with 4 processes. You can adjust this to your preference.

The first argument after the program name must be either "sign" or "verify":

- In "sign" mode, you provide the path to a file as input, and the program will compute the PMAC tag for this file and write it to a new file in the same directory.

  For example:

  ```sh
  mpirun -np 4 pmac sign input.txt
  ```

  This will create a file named `input.txt.tag` in the same directory as `input.txt`, containing the computed PMAC tag.

- In "verify" mode, you provide the path to a file and the path to a file containing a previously computed PMAC tag, and the program will compute the PMAC tag for the file again and compare it to the provided tag.

  For example:

  ```sh
  mpirun -np 4 pmac verify input.txt input.txt.tag
  ```

  The program will output either "Tag verification succeeded" if the computed tag matches the provided tag, or "Tag verification failed" otherwise.

## Note

This is a basic implementation of PMAC and is not suitable for handling sensitive data or use in a production environment.

In particular, the secret keys used for the PMAC computations are hardcoded and not secret at all, and the functions F1 and F2, which in a real PMAC implementation would be cryptographic pseudorandom functions, are here simple XOR operations.

Moreover, this program does not handle errors robustly and is not optimized for performance. Its purpose is to illustrate how PMAC and MPI can be used together to compute a MAC in parallel.
