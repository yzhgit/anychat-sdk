//
//  File.swift
//  AnyChatSDK
//
//  File manager with async/await support
//

import Foundation

public actor FileManager {
    private let handle: AnyChatFileHandle

    init(handle: AnyChatFileHandle) {
        self.handle = handle
    }

    // MARK: - File Operations

    public func upload(
        localPath: String,
        fileType: String,
        onProgress: ((Int64, Int64) -> Void)? = nil
    ) async throws -> FileInfo {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            // Progress callback wrapper
            let progressCallback: AnyChatUploadProgressCallback? = onProgress != nil ? { userdata, uploaded, total in
                if let progress = onProgress {
                    progress(uploaded, total)
                }
            } : nil

            // Completion callback
            let doneCallback: AnyChatFileInfoCallback = { userdata, success, info, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<FileInfo>>.fromOpaque(userdata).takeRetainedValue()

                if success != 0, let info = info?.pointee {
                    context.continuation.resume(returning: FileInfo(from: info))
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Upload failed"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            withCString(localPath) { pathPtr in
                withCString(fileType) { typePtr in
                    let result = anychat_file_upload(
                        handle,
                        pathPtr,
                        typePtr,
                        userdata,
                        progressCallback,
                        doneCallback
                    )

                    if result != ANYCHAT_OK {
                        let ctx = Unmanaged<CallbackContext<FileInfo>>.fromOpaque(userdata).takeRetainedValue()
                        ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
                    }
                }
            }
        }
    }

    public func getDownloadURL(fileId: String) async throws -> String {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatDownloadUrlCallback = { userdata, success, url, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<String>>.fromOpaque(userdata).takeRetainedValue()

                if success != 0, let url = url {
                    context.continuation.resume(returning: String(cString: url))
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Failed to get download URL"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            withCString(fileId) { fileIdPtr in
                let result = anychat_file_get_download_url(
                    handle,
                    fileIdPtr,
                    userdata,
                    callback
                )

                if result != ANYCHAT_OK {
                    let ctx = Unmanaged<CallbackContext<String>>.fromOpaque(userdata).takeRetainedValue()
                    ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
                }
            }
        }
    }

    public func delete(fileId: String) async throws {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatFileCallback = { userdata, success, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()

                if success != 0 {
                    context.continuation.resume(returning: ())
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Delete failed"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            withCString(fileId) { fileIdPtr in
                let result = anychat_file_delete(handle, fileIdPtr, userdata, callback)

                if result != ANYCHAT_OK {
                    let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                    ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
                }
            }
        }
    }
}
