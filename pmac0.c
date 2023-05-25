#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define P 4294967291 // Large prime number for modular arithmetic

// One Time Pad-like function for computing intermediate values in PMAC
uint64_t F1(uint64_t k1, uint64_t r)
{
  return r ^ k1;
}

// One Time Pad-like function for computing final tag in PMAC
uint64_t F2(uint64_t k2, uint64_t t)
{
  return t ^ k2;
}

// Parallelizable version of PMAC (Parallelizable MAC)
void PMAC0(uint64_t k,             // secret key
           uint64_t k1,            // secret key for F1
           uint64_t k2,            // secret key for F2
           FILE *file,             // file to compute tag for
           long file_length,       // length of file
           uint64_t *tag)          // output: tag
{
  uint64_t t = 0;  // intermediate tag value
  uint64_t mask = 0; // mask for computing current block
  uint64_t r; // current block
  uint64_t m; // current byte

  int rank; // rank of current process
  int size; // total number of processes

  long i = 0; // index for current process

  // get current process's rank and total number of processes
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  // jump to the part of the file this process is responsible for
  fseek(file, rank, SEEK_SET);

  while(i < file_length) // each process reads every size-th byte
  {
    m = fgetc(file);
    if (m == EOF)
      break;

    mask = (mask + k) % P;
    r = (m + mask) % P;
    t ^= F1(k1, r);

    i += size;
    fseek(file, rank + i, SEEK_SET);
  }

  // Padding handling for the PMAC scheme:
  // only the last process (rank == size - 1) should add padding
  if (rank == size - 1)
  {
    m = 0x80; // append a 1 byte

    mask = (mask + k) % P;
    r = (m + mask) % P;
    t ^= F1(k1, r);

    // Pad with 0 bytes so that total number of blocks (including padding)
    // is divisible by the number of processes
    long pad_length = (size - file_length % size) % size;
    for (long j = 0; j < pad_length; ++j)
    {
      m = 0;
      mask = (mask + k) % P;
      r = (m + mask) % P;
      t ^= F1(k1, r);
    }
  }

  // Combine all the intermediate tags from all processes
  MPI_Reduce(&t, tag, 1, MPI_UNSIGNED_LONG_LONG, MPI_BXOR, 0, MPI_COMM_WORLD);

  // Final tag computation
  if (rank == 0)
    *tag = F2(k2, *tag);
}

int main(int argc, char** argv)
{
  if (argc < 3 || argc > 4)
  {
    fprintf(stderr, "Usage: %s <sign|verify> <filepath> [<tagfilepath>]\n", argv[0]);
    return 1;
  }

  uint64_t k  = 123456;
  uint64_t k1 = 234567;
  uint64_t k2 = 345678;

  uint64_t tag;

  MPI_Init(NULL, NULL);

  FILE *file = fopen(argv[2], "rb");
  if (!file)
  {
    perror("Failed to open file");
    MPI_Finalize();
    return 1;
  }

  fseek(file, 0, SEEK_END);
  long file_length = ftell(file);
  rewind(file);

  PMAC0(k, k1, k2, file, file_length, &tag);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  MPI_Finalize();

  if (rank == 0)
  {
    if (strcmp(argv[1], "sign") == 0)
    {
      // "sign" mode
      char tagfilename[512];
      sprintf(tagfilename, "%s.tag", argv[2]);

      FILE *tagfile = fopen(tagfilename, "w");
      if (!tagfile)
      {
        perror("Failed to open tag file for writing");
        return 1;
      }

      fprintf(tagfile, "%llu\n", tag);
      fclose(tagfile);

      printf("Tag written to: %s\n", tagfilename);
    }
    else if (strcmp(argv[1], "verify") == 0)
    {
      // "verify" mode
      if (argc != 4)
      {
        fprintf(stderr, "Usage: %s verify <filepath> <tagfilepath>\n", argv[0]);
        return 1;
      }

      FILE *tagfile = fopen(argv[3], "r");
      if (!tagfile)
      {
        perror("Failed to open tag file for reading");
        return 1;
      }

      uint64_t old_tag;
      if (fscanf(tagfile, "%llu", &old_tag) != 1)
      {
        fprintf(stderr, "Failed to read tag from tag file\n");
        return 1;
      }

      if (tag == old_tag)
      { printf("Tag verification succeeded\n"); }
      else
      { printf("Tag verification failed\n"); }
    }
    else
    {
      fprintf(stderr, "Invalid mode. Please use either 'sign' or 'verify'\n");
      return 1;
    }
  }

  fclose(file);

  return 0;
}
