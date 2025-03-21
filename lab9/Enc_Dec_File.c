#include <windows.h>
#include <stdio.h>
#include <bcrypt.h>

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)

static const BYTE rgbIV[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

static const BYTE rgbAES128Key[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

void PrintBytes(IN BYTE *pbPrintData, IN DWORD cbDataLen) {
    for (DWORD dwCount = 0; dwCount < cbDataLen; dwCount++) {
        printf("0x%02x, ", pbPrintData[dwCount]);
        if (0 == (dwCount + 1) % 10) putchar('\n');
    }
}

BOOL ReadFileData(char *filename, PBYTE *pbData, DWORD *cbData) {
    HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("**** Error opening file %s\n", filename);
        return FALSE;
    }

    *cbData = GetFileSize(hFile, NULL);
    *pbData = (PBYTE)HeapAlloc(GetProcessHeap(), 0, *cbData);
    if (*pbData == NULL) {
        printf("**** memory allocation failed\n");
        CloseHandle(hFile);
        return FALSE;
    }

    DWORD dwBytesRead;
    if (!ReadFile(hFile, *pbData, *cbData, &dwBytesRead, NULL)) {
        printf("**** Error reading file %s\n", filename);
        HeapFree(GetProcessHeap(), 0, *pbData);
        CloseHandle(hFile);
        return FALSE;
    }

    CloseHandle(hFile);
    return TRUE;
}

BOOL WriteFileData(char *filename, PBYTE pbData, DWORD cbData) {
    HANDLE hFile = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("**** Error creating file %s\n", filename);
        return FALSE;
    }

    DWORD dwBytesWritten;
    if (!WriteFile(hFile, pbData, cbData, &dwBytesWritten, NULL)) {
        printf("**** Error writing to file %s\n", filename);
        CloseHandle(hFile);
        return FALSE;
    }

    CloseHandle(hFile);
    return TRUE;
}

void __cdecl main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <encrypt/decrypt> <input file> <output file>\n", argv[0]);
        return;
    }

    BOOL bEncrypt = ((strcmp(argv[1], "encrypt") == 0));
    BOOL bDecrypt = (strcmp(argv[1], "decrypt") == 0);

    if (!bEncrypt && !bDecrypt) {
        printf("Invalid command. Use 'encrypt' or 'decrypt'.\n");
        return;
    }

    BCRYPT_ALG_HANDLE hAesAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    DWORD cbCipherText = 0, cbPlainText = 0, cbData = 0, cbKeyObject = 0, cbBlockLen = 0;
    PBYTE pbCipherText = NULL, pbPlainText = NULL, pbKeyObject = NULL, pbIV = NULL;

    // Open an algorithm handle.
    if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider(&hAesAlg, BCRYPT_AES_ALGORITHM, NULL, 0))) {
        printf("**** Error 0x%x returned by BCryptOpenAlgorithmProvider\n", status);
        goto Cleanup;
    }

    // Calculate the size of the buffer to hold the KeyObject.
    if (!NT_SUCCESS(status = BCryptGetProperty(hAesAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbKeyObject, sizeof(DWORD), &cbData, 0))) {
        printf("**** Error 0x%x returned by BCryptGetProperty\n", status);
        goto Cleanup;
    }

    // Allocate the key object on the heap.
    pbKeyObject = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbKeyObject);
    if (NULL == pbKeyObject) {
        printf("**** memory allocation failed\n");
        goto Cleanup;
    }

    // Calculate the block length for the IV.
    if (!NT_SUCCESS(status = BCryptGetProperty(hAesAlg, BCRYPT_BLOCK_LENGTH, (PBYTE)&cbBlockLen, sizeof(DWORD), &cbData, 0))) {
        printf("**** Error 0x%x returned by BCryptGetProperty\n", status);
        goto Cleanup;
    }

    // Allocate a buffer for the IV.
    pbIV = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbBlockLen);
    if (NULL == pbIV) {
        printf("**** memory allocation failed\n");
        goto Cleanup;
    }

    memcpy(pbIV, rgbIV, cbBlockLen);

    // Set the chaining mode to CBC.
    if (!NT_SUCCESS(status = BCryptSetProperty(hAesAlg, BCRYPT_CHAINING_MODE, (PBYTE)BCRYPT_CHAIN_MODE_CBC, sizeof(BCRYPT_CHAIN_MODE_CBC), 0))) {
        printf("**** Error 0x%x returned by BCryptSetProperty\n", status);
        goto Cleanup;
    }

    // Generate the key from supplied input key bytes.
    if (!NT_SUCCESS(status = BCryptGenerateSymmetricKey(hAesAlg, &hKey, pbKeyObject, cbKeyObject, (PBYTE)rgbAES128Key, sizeof(rgbAES128Key), 0))) {
        printf("**** Error 0x%x returned by BCryptGenerateSymmetricKey\n", status);
        goto Cleanup;
    }

    // Read the input file.
    if (!ReadFileData(argv[2], &pbPlainText, &cbPlainText)) {
        goto Cleanup;
    }

    if (bEncrypt) {
        // Get the output buffer size.
        if (!NT_SUCCESS(status = BCryptEncrypt(hKey, pbPlainText, cbPlainText, NULL, pbIV, cbBlockLen, NULL, 0, &cbCipherText, BCRYPT_BLOCK_PADDING))) {
            printf("**** Error 0x%x returned by BCryptEncrypt\n", status);
            goto Cleanup;
        }

        pbCipherText = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbCipherText);
        if (NULL == pbCipherText) {
            printf("**** memory allocation failed\n");
            goto Cleanup;
        }

        // Encrypt the data.
        if (!NT_SUCCESS(status = BCryptEncrypt(hKey, pbPlainText, cbPlainText, NULL, pbIV, cbBlockLen, pbCipherText, cbCipherText, &cbData, BCRYPT_BLOCK_PADDING))) {
            printf("**** Error 0x%x returned by BCryptEncrypt\n", status);
            goto Cleanup;
        }

        // Write the encrypted data to the output file.
        if (!WriteFileData(argv[3], pbCipherText, cbCipherText)) {
            goto Cleanup;
        }

        printf("File encrypted successfully.\n");
    } else if (bDecrypt) {
        // Get the output buffer size.
        if (!NT_SUCCESS(status = BCryptDecrypt(hKey, pbPlainText, cbPlainText, NULL, pbIV, cbBlockLen, NULL, 0, &cbPlainText, BCRYPT_BLOCK_PADDING))) {
            printf("**** Error 0x%x returned by BCryptDecrypt\n", status);
            goto Cleanup;
        }

        PBYTE pbDecryptedText = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbPlainText);
        if (NULL == pbDecryptedText) {
            printf("**** memory allocation failed\n");
            goto Cleanup;
        }

        // Decrypt the data.
        if (!NT_SUCCESS(status = BCryptDecrypt(hKey, pbPlainText, cbPlainText, NULL, pbIV, cbBlockLen, pbDecryptedText, cbPlainText, &cbPlainText, BCRYPT_BLOCK_PADDING))) {
            printf("**** Error 0x%x returned by BCryptDecrypt\n", status);
            HeapFree(GetProcessHeap(), 0, pbDecryptedText);
            goto Cleanup;
        }

        // Write the decrypted data to the output file.
        if (!WriteFileData(argv[3], pbDecryptedText, cbPlainText)) {
            HeapFree(GetProcessHeap(), 0, pbDecryptedText);
            goto Cleanup;
        }

        HeapFree(GetProcessHeap(), 0, pbDecryptedText);
        printf("File decrypted successfully.\n");
    }

Cleanup:
    if (hAesAlg) {
        BCryptCloseAlgorithmProvider(hAesAlg, 0);
    }
    if (hKey) {
        BCryptDestroyKey(hKey);
    }
    if (pbCipherText) {
        HeapFree(GetProcessHeap(), 0, pbCipherText);
    }
    if (pbPlainText) {
        HeapFree(GetProcessHeap(), 0, pbPlainText);
    }
    if (pbKeyObject) {
        HeapFree(GetProcessHeap(), 0, pbKeyObject);
    }
    if (pbIV) {
        HeapFree(GetProcessHeap(), 0, pbIV);
    }
}