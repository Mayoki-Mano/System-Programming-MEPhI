#include <windows.h>
#include <stdio.h>
#include <bcrypt.h>
#include <ncrypt.h>

#define NT_SUCCESS(Status)          (((NTSTATUS)(Status)) >= 0)

#define STATUS_UNSUCCESSFUL         ((NTSTATUS)0xC0000001L)

// Function to sign hash
BOOL SignHash(PBYTE pbHash, DWORD cbHash, NCRYPT_KEY_HANDLE hKey, PBYTE *pbSignature, DWORD cbSignature) {
    SECURITY_STATUS secStatus = ERROR_SUCCESS;

    // Allocate memory for the signature
    *pbSignature = (PBYTE) HeapAlloc(GetProcessHeap(), 0, cbSignature);
    if (*pbSignature == NULL) {
        printf("**** memory allocation failed\n");
        return FALSE;
    }

    // Second call to get the signature
    secStatus = NCryptSignHash(
        hKey,
        NULL, // No padding info for ECDSA
        pbHash,
        cbHash,
        *pbSignature,
        cbSignature,
        &cbSignature,
        0);
    if (secStatus != ERROR_SUCCESS) {
        printf("**** Error 0x%x returned by NCryptSignHash (sign)\n", secStatus);
        HeapFree(GetProcessHeap(), 0, *pbSignature);
        *pbSignature = NULL; // Убедимся, что указатель сброшен
        return FALSE;
    }

    return TRUE;
}

BOOL VerifySignature(PBYTE hash, DWORD hashSize, PBYTE signature, DWORD signatureSize, NCRYPT_KEY_HANDLE hKey) {
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // Проверяем подпись
    status = NCryptVerifySignature(
        hKey,
        NULL, // Нет информации о заполнении для ECDSA
        hash,
        hashSize,
        signature,
        signatureSize,
        0); // Нет флагов для ECDSA

    if (status == NTE_BAD_SIGNATURE) {
        printf("**** Error: Wrong signature ;(\n");
        return FALSE;
    }
    if (status != ERROR_SUCCESS) {
        printf("Error verifying signature: 0x%x\n", status);
        return FALSE;
    }
    return TRUE;
}

void PrintHex(BYTE *data, DWORD size) {
    for (DWORD i = 0; i < size; i++) {
        printf("%02X", data[i]);
    }
    printf("\n");
}

BOOL CalculateFileHashWithHandle(BCRYPT_HASH_HANDLE hHash, PBYTE hash, DWORD hashSize, LPCSTR filename,
                                 DWORD cbSignature, BOOL isSigned) {
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD cbRead = 0;
    BYTE buffer[4096];
    BOOL bResult = FALSE;

    // Open the file
    hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Failed to open file.\n");
        goto cleanup;
    }

    // Get the file size
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        printf("Failed to get file size.\n");
        goto cleanup;
    }

    // Calculate the size of the file without the signature
    LARGE_INTEGER dataSize;
    if (isSigned)
        dataSize.QuadPart = fileSize.QuadPart - cbSignature;
    else
        dataSize.QuadPart = fileSize.QuadPart;

    // Read the file and update hash
    while (dataSize.QuadPart > 0) {
        DWORD bytesToRead = (dataSize.QuadPart > sizeof(buffer)) ? sizeof(buffer) : (DWORD) dataSize.QuadPart;
        if (!ReadFile(hFile, buffer, bytesToRead, &cbRead, NULL)) {
            printf("Failed to read file.\n");
            goto cleanup;
        }

        if (!NT_SUCCESS(BCryptHashData(hHash, buffer, cbRead, 0))) {
            printf("Failed to hash data.\n");
            goto cleanup;
        }

        dataSize.QuadPart -= cbRead;
    }

    // Finalize the hash
    if (!NT_SUCCESS(BCryptFinishHash(hHash, hash, hashSize, 0))) {
        printf("Failed to finish hash.\n");
        goto cleanup;
    }

    bResult = TRUE;

cleanup:
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }

    return bResult;
}

void PrintHash(PBYTE hash, DWORD hashSize) {
    printf("Hash: ");
    for (DWORD i = 0; i < hashSize; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");
}

void PrintSignature(PBYTE signature, DWORD signatureSize) {
    printf("Signature: ");
    for (DWORD i = 0; i < signatureSize; i++) {
        printf("%02x", signature[i]);
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: %s <sign|verify> <file> <key>\n", argv[0]);
        return 1;
    }

    const char *command = argv[1];
    const char *filePath = argv[2];
    const char *keyName = argv[3];
    WCHAR wideKeyName[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, keyName, -1, wideKeyName, MAX_PATH);

    NCRYPT_PROV_HANDLE hProv = NULL;
    NCRYPT_KEY_HANDLE hKey = NULL;
    BCRYPT_ALG_HANDLE hHashAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    PBYTE pbHash = NULL;
    PBYTE pbSignature = NULL;
    PBYTE pbKeyBlob = NULL;
    DWORD cbHash = 0;
    DWORD cbSignature = 0;
    DWORD cbKeyBlob = 0;
    BOOL bResult = FALSE;

    // Open the key storage provider
    SECURITY_STATUS secStatus = NCryptOpenStorageProvider(&hProv, MS_KEY_STORAGE_PROVIDER, 0);
    if (secStatus != ERROR_SUCCESS) {
        printf("**** Error 0x%x returned by NCryptOpenStorageProvider\n", secStatus);
        goto Cleanup;
    }

    // Open the hash algorithm provider
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hHashAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0);
    if (!NT_SUCCESS(status)) {
        printf("**** Error 0x%x returned by BCryptOpenAlgorithmProvider\n", status);
        goto Cleanup;
    }

    // Get the hash size
    DWORD cbData = 0;
    status = BCryptGetProperty(hHashAlg, BCRYPT_HASH_LENGTH, (PBYTE) &cbHash, sizeof(DWORD), &cbData, 0);
    if (!NT_SUCCESS(status)) {
        printf("**** Error 0x%x returned by BCryptGetProperty\n", status);
        goto Cleanup;
    }

    // Allocate memory for the hash
    pbHash = (PBYTE) HeapAlloc(GetProcessHeap(), 0, cbHash);
    if (pbHash == NULL) {
        printf("**** memory allocation failed\n");
        goto Cleanup;
    }

    // Create the hash object
    status = BCryptCreateHash(hHashAlg, &hHash, NULL, 0, NULL, 0, 0);
    if (!NT_SUCCESS(status)) {
        printf("**** Error 0x%x returned by BCryptCreateHash\n", status);
        goto Cleanup;
    }

    if (strcmp(command, "sign") == 0) {
        // Create or open the key
        secStatus = NCryptCreatePersistedKey(hProv, &hKey, NCRYPT_ECDSA_P256_ALGORITHM, wideKeyName, 0,
                                             NCRYPT_OVERWRITE_KEY_FLAG);
        if (secStatus != ERROR_SUCCESS) {
            printf("**** Error 0x%x returned by NCryptCreatePersistedKey\n", secStatus);
            goto Cleanup;
        }

        // Finalize the key
        secStatus = NCryptFinalizeKey(hKey, 0);
        if (secStatus != ERROR_SUCCESS) {
            printf("**** Error 0x%x returned by NCryptFinalizeKey\n", secStatus);
            goto Cleanup;
        }

        // Get the size of the exported key
        secStatus = NCryptExportKey(hKey, NULL, BCRYPT_ECCPUBLIC_BLOB, NULL, NULL, 0, &cbKeyBlob, 0);
        if (secStatus != ERROR_SUCCESS) {
            printf("**** Error 0x%x returned by NCryptExportKey (size)\n", secStatus);
            goto Cleanup;
        }

        // Allocate memory for the exported key
        pbKeyBlob = (PBYTE) HeapAlloc(GetProcessHeap(), 0, cbKeyBlob);
        if (pbKeyBlob == NULL) {
            printf("**** memory allocation failed\n");
            goto Cleanup;
        }

        // Export the key
        secStatus = NCryptExportKey(hKey, NULL, BCRYPT_ECCPUBLIC_BLOB, NULL, pbKeyBlob, cbKeyBlob, &cbKeyBlob, 0);
        if (secStatus != ERROR_SUCCESS) {
            printf("**** Error 0x%x returned by NCryptExportKey\n", secStatus);
            goto Cleanup;
        }

        // Print the exported key in hex format
        printf("Exported public key (hex): ");
        PrintHex(pbKeyBlob, cbKeyBlob);

        // Get the signature size
        secStatus = NCryptSignHash(
            hKey,
            NULL, // No padding info for ECDSA
            pbHash,
            cbHash,
            NULL,
            0,
            &cbSignature,
            0);
        if (secStatus != ERROR_SUCCESS) {
            printf("**** Error 0x%x returned by NCryptSignHash (size)\n", secStatus);
            goto Cleanup;
        }

        // Calculate the file hash
        if (!CalculateFileHashWithHandle(hHash, pbHash, cbHash, filePath, 0, FALSE)) {
            goto Cleanup;
        }

        // Print the hash for debugging
        PrintHash(pbHash, cbHash);

        // Sign the hash
        if (!SignHash(pbHash, cbHash, hKey, &pbSignature, cbSignature)) {
            goto Cleanup;
        }

        // Print the signature for debugging
        PrintSignature(pbSignature, cbSignature);

        // Create a new file with the .sig extension
        char sigFilePath[MAX_PATH];
        snprintf(sigFilePath, MAX_PATH, "%s.sig", filePath);

        // Copy the original file content to the new file
        HANDLE hOriginalFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                           FILE_ATTRIBUTE_NORMAL, NULL);
        if (hOriginalFile == INVALID_HANDLE_VALUE) {
            printf("Failed to open original file: %s\n", filePath);
            goto Cleanup;
        }

        HANDLE hSigFile = CreateFileA(sigFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hSigFile == INVALID_HANDLE_VALUE) {
            printf("Failed to create signature file: %s\n", sigFilePath);
            CloseHandle(hOriginalFile);
            goto Cleanup;
        }

        // Copy the original file content
        BYTE buffer[4096];
        DWORD bytesRead, bytesWritten;
        while (ReadFile(hOriginalFile, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
            if (!WriteFile(hSigFile, buffer, bytesRead, &bytesWritten, NULL)) {
                printf("Failed to write to signature file: %s\n", sigFilePath);
                CloseHandle(hOriginalFile);
                CloseHandle(hSigFile);
                goto Cleanup;
            }
        }

        // Append the signature to the end of the new file
        if (!WriteFile(hSigFile, pbSignature, cbSignature, &bytesWritten, NULL)) {
            printf("Failed to write signature to file: %s\n", sigFilePath);
            CloseHandle(hOriginalFile);
            CloseHandle(hSigFile);
            goto Cleanup;
        }

        CloseHandle(hOriginalFile);
        CloseHandle(hSigFile);
        printf("Signature saved to file: %s\n", sigFilePath);
    } else if (strcmp(command, "verify") == 0) {
        secStatus = NCryptOpenKey(hProv, &hKey, wideKeyName, 0, 0);
        if (secStatus != ERROR_SUCCESS) {
            printf("**** Error 0x%x returned by NCryptOpenKey\n", secStatus);
            goto Cleanup;
        }

        // Get the size of the exported key
        secStatus = NCryptExportKey(hKey, NULL, BCRYPT_ECCPUBLIC_BLOB, NULL, NULL, 0, &cbKeyBlob, 0);
        if (secStatus != ERROR_SUCCESS) {
            printf("**** Error 0x%x returned by NCryptExportKey (size)\n", secStatus);
            goto Cleanup;
        }

        // Allocate memory for the exported key
        pbKeyBlob = (PBYTE) HeapAlloc(GetProcessHeap(), 0, cbKeyBlob);
        if (pbKeyBlob == NULL) {
            printf("**** memory allocation failed\n");
            goto Cleanup;
        }

        // Export the key
        secStatus = NCryptExportKey(hKey, NULL, BCRYPT_ECCPUBLIC_BLOB, NULL, pbKeyBlob, cbKeyBlob, &cbKeyBlob, 0);
        if (secStatus != ERROR_SUCCESS) {
            printf("**** Error 0x%x returned by NCryptExportKey\n", secStatus);
            goto Cleanup;
        }

        // Print the exported key in hex format
        printf("Exported public key (hex): ");
        PrintHex(pbKeyBlob, cbKeyBlob);

        // Get the signature size
        secStatus = NCryptSignHash(
            hKey,
            NULL, // No padding info for ECDSA
            pbHash,
            cbHash,
            NULL,
            0,
            &cbSignature,
            0);
        if (secStatus != ERROR_SUCCESS) {
            printf("**** Error 0x%x returned by NCryptSignHash (size)\n", secStatus);
            goto Cleanup;
        }

        // Calculate the file hash (excluding the signature)
        if (!CalculateFileHashWithHandle(hHash, pbHash, cbHash, filePath, cbSignature, TRUE)) {
            goto Cleanup;
        }

        // Print the hash for debugging
        PrintHash(pbHash, cbHash);

        // Open the file to read the signature from the end
        HANDLE hFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                   NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            printf("Failed to open file for reading: %s\n", filePath);
            goto Cleanup;
        }

        // Get the file size
        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(hFile, &fileSize)) {
            printf("Failed to get file size.\n");
            CloseHandle(hFile);
            goto Cleanup;
        }

        // Allocate memory for the signature
        pbSignature = (PBYTE) HeapAlloc(GetProcessHeap(), 0, cbSignature);
        if (pbSignature == NULL) {
            printf("**** memory allocation failed\n");
            CloseHandle(hFile);
            goto Cleanup;
        }

        // Move to the end of the file minus the signature size
        LARGE_INTEGER offset;
        offset.QuadPart = fileSize.QuadPart - cbSignature;
        if (!SetFilePointerEx(hFile, offset, NULL, FILE_BEGIN)) {
            printf("Failed to set file pointer.\n");
            CloseHandle(hFile);
            goto Cleanup;
        }

        // Read the signature
        DWORD bytesRead;
        if (!ReadFile(hFile, pbSignature, cbSignature, &bytesRead, NULL)) {
            printf("Failed to read signature from file.\n");
            CloseHandle(hFile);
            goto Cleanup;
        }

        CloseHandle(hFile);

        // Print the signature for debugging
        PrintSignature(pbSignature, cbSignature);

        // Verify the signature
        if (VerifySignature(pbHash, cbHash, pbSignature, cbSignature, hKey)) {
            printf("Signature is valid.\n");
        } else {
            printf("Signature is invalid.\n");
        }
    } else {
        printf("Unknown command: %s\n", command);
    }

    bResult = TRUE;

Cleanup:
    if (hHash) {
        BCryptDestroyHash(hHash);
    }
    if (hHashAlg) {
        BCryptCloseAlgorithmProvider(hHashAlg, 0);
    }
    if (hKey) {
        NCryptFreeObject(hKey);
    }
    if (hProv) {
        NCryptFreeObject(hProv);
    }
    if (pbHash) {
        HeapFree(GetProcessHeap(), 0, pbHash);
    }
    if (pbSignature) {
        HeapFree(GetProcessHeap(), 0, pbSignature);
    }
    if (pbKeyBlob) {
        HeapFree(GetProcessHeap(), 0, pbKeyBlob);
    }

    return bResult ? 0 : 1;
}