#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

    typedef struct
    {
        char* data;
        SIZE_T size;
        SIZE_T reserved, committed;
    } VMEMO;

    /// \brief Initialize a \c VMEMO object and reserve (but not commit) virtual memory.
    /// \param memo Virtual memory object.
    /// \param maxsize Reserved memory size. Data written to the virtual memory object must not exceed this limit.
    /// \return Win32 error code.
    static ULONG VirtualMemoryInitialize(_Inout_ VMEMO* memo, _In_ SIZE_T maxsize)
    {
        if (memo)
        {
            memset(memo, 0, sizeof(*memo));
            memo->data = (char*)VirtualAlloc(NULL, maxsize, MEM_RESERVE, PAGE_READWRITE);
            if (memo->data)
            {
                MEMORY_BASIC_INFORMATION mbi = { 0 };
                memo->reserved = (VirtualQuery(memo->data, &mbi, sizeof(mbi)) > 0) ? mbi.RegionSize : maxsize;
            }
            else
            {
                return GetLastError();
            }
        }
        return ERROR_SUCCESS;
    }

    /// \brief Append data to \c VMEMO object.
    /// \details More virtual memory is commited if necessary.
    /// \param memo Virtual memory object.
    /// \param data Data pointer.
    /// \param size Data size.
    /// \return Number of written bytes. A value equal to \c size indicates success.
    static SIZE_T VirtualMemoryAppend(_Inout_ VMEMO* memo, _In_ const void* data, _In_ SIZE_T size)
    {
        ULONG err = ERROR_SUCCESS;
        if (memo && memo->data && data && size)
        {
            // Commit more
            if (memo->size + size > memo->committed)
            {
                SYSTEM_INFO si;
                GetSystemInfo(&si);

                ULONG64 grow = 0;
                grow = (memo->committed * 20) / 100;				// xx% of existing size
                grow = max(grow, size);								// ...at least the size of new data
                if (grow % si.dwPageSize)
                {
                    grow += si.dwPageSize - (grow % si.dwPageSize);	    // round up to page size (usually 4KB)
                }
                grow = min(grow, UINTPTR_MAX - memo->committed);    // don't overflow

                memo->committed = min(memo->committed + (SIZE_T)grow, memo->reserved);
                err = VirtualAlloc(memo->data, memo->committed, MEM_COMMIT, PAGE_READWRITE) ? ERROR_SUCCESS : GetLastError();

#define VMEMO_SIZE(bytes) ((bytes) > 1024 * 1024 ? (bytes)/1024/1024 : (bytes)/1024), ((bytes) > 1024*1024 ? "mb" : "kb")
                // printf("-- VirtualAlloc( size:%llu%s, committed:%llu%s, reserved:%llu%s ) = 0x%lx\n", VMEMO_SIZE(memo->size), VMEMO_SIZE(memo->committed), VMEMO_SIZE(memo->reserved), err);
#undef VMEMO_SIZE
            }

            // Write
            if (err == ERROR_SUCCESS)
            {
                SIZE_T bytes = min(size, memo->reserved - memo->size);
                if (bytes > 0)
                {
                    memcpy(memo->data + memo->size, data, bytes);
                    memo->size += bytes;
                }
                return bytes;
            }
        }
        return 0;
    }

    /// \brief Decommit all virtual memory but keep it reserved for reuse.
    static void VirtualMemoryReset(_Inout_ VMEMO* memo)
    {
        if (memo)
        {
            if (memo->data)
                VirtualFree(memo->data, memo->reserved, MEM_DECOMMIT);
            memo->committed = memo->size = 0;
        }
    }

    /// \brief Release all virtual memory.
    /// \details The \c VMEMO object cannot be reused unless \ref VirtualMemoryInitialize is called again.
    static void VirtualMemoryDestroy(_Inout_ VMEMO* memo)
    {
        if (memo)
        {
            if (memo->data)
                VirtualFree(memo->data, 0, MEM_RELEASE);
            memset(memo, 0, sizeof(*memo));
        }
    }

#if defined(__cplusplus)
}
#endif
