//
//  File.swift
//  AnyChatSDK
//
//  File manager with async/await support
//

import Foundation

private final class FileUploadContext: @unchecked Sendable {
    let continuation: CheckedContinuation<FileInfo, Error>
    let onProgress: ((Int64, Int64) -> Void)?

    init(
        continuation: CheckedContinuation<FileInfo, Error>,
        onProgress: ((Int64, Int64) -> Void)?
    ) {
        self.continuation = continuation
        self.onProgress = onProgress
    }
}

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
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<FileInfo, Error>) in
            let context = FileUploadContext(
                continuation: continuation,
                onProgress: onProgress
            )
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var doneCallback = AnyChatFileInfoCallback_C()
            doneCallback.struct_size = UInt32(MemoryLayout<AnyChatFileInfoCallback_C>.size)
            doneCallback.userdata = userdata
            doneCallback.on_success = { cbUserdata, info in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<FileUploadContext>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let info else {
                    ctx.continuation.resume(throwing: AnyChatError.network)
                    return
                }
                ctx.continuation.resume(returning: FileInfo(from: info.pointee))
            }
            doneCallback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<FileUploadContext>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let progressCallback: AnyChatUploadProgressCallback? = { cbUserdata, uploaded, total in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<FileUploadContext>.fromOpaque(cbUserdata).takeUnretainedValue()
                ctx.onProgress?(uploaded, total)
            }

            let result = withCString(localPath) { pathPtr in
                withCString(fileType) { typePtr in
                    anychat_file_upload(
                        handle,
                        pathPtr,
                        typePtr,
                        progressCallback,
                        &doneCallback
                    )
                }
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<FileUploadContext>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func getDownloadURL(fileId: String) async throws -> String {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<String, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatDownloadUrlCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatDownloadUrlCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata, url in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<String>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let url else {
                    ctx.continuation.resume(throwing: AnyChatError.network)
                    return
                }
                ctx.continuation.resume(returning: String(cString: url))
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<String>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = withCString(fileId) { fileIdPtr in
                anychat_file_get_download_url(handle, fileIdPtr, &callback)
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<String>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func delete(fileId: String) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatFileCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatFileCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(cbUserdata).takeRetainedValue()
                ctx.continuation.resume(returning: ())
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = withCString(fileId) { fileIdPtr in
                anychat_file_delete(handle, fileIdPtr, &callback)
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }
}
