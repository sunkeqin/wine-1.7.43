@ stdcall CancelIo(long) kernel32.CancelIo
@ stdcall CancelIoEx(long ptr) kernel32.CancelIoEx
@ stub CancelSynchronousIo
@ stdcall CreateIoCompletionPort(long long long long) kernel32.CreateIoCompletionPort
@ stdcall DeviceIoControl(long long ptr long ptr long ptr ptr) kernel32.DeviceIoControl
@ stdcall GetOverlappedResult(long ptr ptr long) kernel32.GetOverlappedResult
@ stub GetOverlappedResultEx
@ stdcall GetQueuedCompletionStatus(long ptr ptr ptr long) kernel32.GetQueuedCompletionStatus
@ stub GetQueuedCompletionStatusEx
@ stdcall PostQueuedCompletionStatus(long long ptr ptr) kernel32.PostQueuedCompletionStatus
