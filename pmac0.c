#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define P 4294967291 // Large prime number for modular arithmetic

// One Time Pad-like function for computing intermediate values in PMAC
uint64_t F1_OTP(uint64_t k1, uint64_t r)
{
  return r ^ k1;
}

// One Time Pad-like function for computing final tag in PMAC
uint64_t F2_OTP(uint64_t k2, uint64_t t)
{
  return t ^ k2;
}

// F1 and F2 using Vigenere cipher
uint64_t F1_vigenere(uint64_t k1, uint64_t r)
{
  uint64_t t = 0;
  uint64_t mask = 0;
  uint64_t m = 0;

  for (int i = 0; i < 8; ++i)
  {
    mask = (mask + k1) % P;
    m = (r >> (8 * i)) & 0xFF;
    t ^= (m + mask) % P;
  }

  return t;
}

uint64_t F2_vigenere(uint64_t k2, uint64_t t)
{
  uint64_t mask = 0;
  uint64_t m = 0;
  uint64_t r = 0;

  for (int i = 0; i < 8; ++i)
  {
    mask = (mask + k2) % P;
    m = (t >> (8 * i)) & 0xFF;
    r ^= (m + mask) % P;
  }

  return r;
}

// F1 and F2 using RC4
uint64_t F1_RC4(uint64_t k1, uint64_t r)
{
  uint64_t t = 0;
  uint64_t mask = 0;
  uint64_t m = 0;

  uint8_t S[256];
  uint8_t T[256];

  // Initialize S and T
  for (int i = 0; i < 256; ++i)
  {
    S[i] = i;
    T[i] = (k1 >> (8 * i)) & 0xFF;
  }

  // Initial permutation of S
  int j = 0;
  for (int i = 0; i < 256; ++i)
  {
    j = (j + S[i] + T[i]) % 256;
    uint8_t temp = S[i];
    S[i] = S[j];
    S[j] = temp;
  }

  // Stream generation
  int i = 0;
  j = 0;
  for (int k = 0; k < 8; ++k)
  {
    i = (i + 1) % 256;
    j = (j + S[i]) % 256;
    uint8_t temp = S[i];
    S[i] = S[j];
    S[j] = temp;

    m = (r >> (8 * k)) & 0xFF;
    t ^= (m + S[(S[i] + S[j]) % 256]) % P;
  }

  return t;
}

uint64_t F2_RC4(uint64_t k2, uint64_t t)
{
  uint64_t mask = 0;
  uint64_t m = 0;
  uint64_t r = 0;

  uint8_t S[256];
  uint8_t T[256];

  // Initialize S and T
  for (int i = 0; i < 256; ++i)
  {
    S[i] = i;
    T[i] = (k2 >> (8 * i)) & 0xFF;
  }

  // Initial permutation of S
  int j = 0;
  for (int i = 0; i < 256; ++i)
  {
    j = (j + S[i] + T[i]) % 256;
    uint8_t temp = S[i];
    S[i] = S[j];
    S[j] = temp;
  }

  // Stream generation
  int i = 0;
  j = 0;
  for (int k = 0; k < 8; ++k)
  {
    i = (i + 1) % 256;
    j = (j + S[i]) % 256;
    uint8_t temp = S[i];
    S[i] = S[j];
    S[j] = temp;

    m = (t >> (8 * k)) & 0xFF;
    r ^= (m + S[(S[i] + S[j]) % 256]) % P;
  }

  return r;
}

// Parallelizable version of PMAC (Parallelizable MAC)
void PMAC0(uint64_t k,             // secret key
           uint64_t k1,            // secret key for F1
           uint64_t k2,            // secret key for F2
           FILE *file,             // file to compute tag for
           long file_length,       // length of file
           uint64_t *tag,          // output: tag
           char *f1f2)             // F1 and F2 selection argument
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

    if (strcmp(f1f2, "otp") == 0) {
      t ^= F1_OTP(k1, r);
    } else if (strcmp(f1f2, "vigenere") == 0) {
      t ^= F1_vigenere(k1, r);
    } else if (strcmp(f1f2, "rc4") == 0) {
      t ^= F1_RC4(k1, r);
    }

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

    if (strcmp(f1f2, "otp") == 0) {
      t ^= F1_OTP(k1, r);
    } else if (strcmp(f1f2, "vigenere") == 0) {
      t ^= F1_vigenere(k1, r);
    } else if (strcmp(f1f2, "rc4") == 0) {
      t ^= F1_RC4(k1, r);
    }

    // Pad with 0 bytes so that total number of blocks (including padding)
    // is divisible by the number of processes
    long pad_length = (size - file_length % size) % size;
    for (long j = 0; j < pad_length; ++j)
    {
      m = 0;
      mask = (mask + k) % P;
      r = (m + mask) % P;

      if (strcmp(f1f2, "otp") == 0) {
        t ^= F1_OTP(k1, r);
      } else if (strcmp(f1f2, "vigenere") == 0) {
        t ^= F1_vigenere(k1, r);
      } else if (strcmp(f1f2, "rc4") == 0) {
        t ^= F1_RC4(k1, r);
      }
    }
  }

  // Combine all the intermediate tags from all processes
  MPI_Reduce(&t, tag, 1, MPI_UNSIGNED_LONG_LONG, MPI_BXOR, 0, MPI_COMM_WORLD);

  // Final tag computation
  if (rank == 0) {
    if (strcmp(f1f2, "otp") == 0) {
      *tag = F2_OTP(k2, *tag);
    } else if (strcmp(f1f2, "vigenere") == 0) {
      *tag = F2_vigenere(k2, *tag);
    } else if (strcmp(f1f2, "rc4") == 0) {
      *tag = F2_RC4(k2, *tag);
    }
  }
}

int main(int argc, char** argv)
{
  if (argc < 4 || argc > 5)
  {
    fprintf(stderr, "Usage: %s <sign|verify> <filepath> <f1f2(otp|vigenere|rc4)> [<tagfilepath>]\n", argv[0]);
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

  PMAC0(k, k1, k2, file, file_length, &tag, argv[3]);

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
        perror("Failed to create tag file");
        return 1;
      }

      fprintf(tagfile, "%llu\n", tag);
      fclose(tagfile);
    }
    else if (strcmp(argv[1], "verify") == 0)
    {
      // "verify" mode
      if (argc < 5)
      {
        fprintf(stderr, "Usage: %s verify <filepath> <f1f2(otp|vigenere|rc4)> <tagfilepath>\n", argv[0]);
        return 1;
      }

      uint64_t expected_tag;
      FILE *expected_tagfile = fopen(argv[4], "r");
      if (!expected_tagfile)
      {
        perror("Failed to open expected tag file");
        return 1;
      }

      fscanf(expected_tagfile, "%llu", &expected_tag);
      fclose(expected_tagfile);

      if (tag == expected_tag)
      {
        printf("Tag is valid\n");
      }
      else
      {
        printf("Tag is NOT valid\n");
      }
    }
  }

  fclose(file);

  return 0;
}
