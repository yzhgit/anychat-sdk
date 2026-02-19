#pragma once

#include "types_c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Callback types ---- */

typedef void (*AnyChatFileCallback)(
    void*       userdata,
    int         success,
    const char* error);

typedef void (*AnyChatFileInfoCallback)(
    void*                   userdata,
    int                     success,
    const AnyChatFileInfo_C* info,
    const char*             error);

/* Progress during upload: uploaded and total are byte counts. */
typedef void (*AnyChatUploadProgressCallback)(
    void*   userdata,
    int64_t uploaded,
    int64_t total);

typedef void (*AnyChatDownloadUrlCallback)(
    void*       userdata,
    int         success,
    const char* url,
    const char* error);

/* ---- File operations ---- */

/* Upload a local file.
 * file_type: "image" | "video" | "audio" | "file"
 * on_progress: may be NULL.
 * on_done: fired when the upload completes (success or failure). */
ANYCHAT_C_API int anychat_file_upload(
    AnyChatFileHandle             handle,
    const char*                   local_path,
    const char*                   file_type,
    void*                         userdata,
    AnyChatUploadProgressCallback on_progress,
    AnyChatFileInfoCallback       on_done);

/* Retrieve a presigned download URL for a file. */
ANYCHAT_C_API int anychat_file_get_download_url(
    AnyChatFileHandle          handle,
    const char*                file_id,
    void*                      userdata,
    AnyChatDownloadUrlCallback callback);

/* Delete a file from the server. */
ANYCHAT_C_API int anychat_file_delete(
    AnyChatFileHandle handle,
    const char*       file_id,
    void*             userdata,
    AnyChatFileCallback callback);

#ifdef __cplusplus
}
#endif
