# DESIGN.md — Project 3: Multi-user CLI Chat Server/Client

**Học viên:** nguyen-quoc-viet | **Ngày nộp:** 2026-07-10

> File này nộp **trước khi mở PR code**. Trọng tâm review ở đây là **protocol** và **cách xử lý epoll/non-blocking I/O** — đây là phần dễ sai nhất và khó sửa nhất nếu phát hiện muộn.

---

## 1. Kiến trúc tổng thể

Sơ đồ luồng xử lý của server (event loop epoll, các loại sự kiện được lắng nghe: `accept`, dữ liệu từ client, stdin admin nếu có...):

```
┌─────────────────────────────────────────────────┐
│          Server Startup                         │
│  - Create listening socket on port 5000         │
│  - Create epoll instance                        │
│  - Add listening socket to epoll (EPOLLIN)     │
└──────────────┬──────────────────────────────────┘
               │
        ┌──────▼──────────────────────┐
        │  Main Event Loop (epoll)    │
        │  - epoll_wait() with -1     │
        │    (blocking forever)       │
        └──────┬──────────────────────┘
               │
        ┌──────▼───────────────────────────────────────┐
        │  Event Processing (for each ready fd):      │
        │                                             │
        │  IF fd == listening socket AND EPOLLIN:    │
        │    ├─ accept() new client                  │
        │    ├─ add to epoll (EPOLLIN | EPOLLOUT)   │
        │    └─ create client_info struct            │
        │                                             │
        │  ELSE IF EPOLLIN (data available):         │
        │    ├─ read() from socket to input_buffer   │
        │    ├─ IF '\n' found in buffer:             │
        │    │   ├─ parse message                    │
        │    │   ├─ route by command type            │
        │    │   └─ queue response to output_buffer  │
        │    └─ ELSE: wait for more data             │
        │                                             │
        │  ELSE IF EPOLLOUT (can write):             │
        │    ├─ write() from output_buffer to socket │
        │    ├─ IF all sent:                         │
        │    │   └─ remove EPOLLOUT flag             │
        │    └─ ELSE: keep EPOLLOUT for next poll    │
        └──────┬──────────────────────────────────────┘
               │
        ┌──────▼──────────────────────┐
        │  Shutdown (on SIGTERM)      │
        │  - Close all client sockets │
        │  - Close epoll instance     │
        │  - Free all buffers/memory  │
        └─────────────────────────────┘
```

## 2. Danh sách Process/Thread (bắt buộc, liệt kê từng cái cụ thể)

| Tên | Loại (process/thread) | Vai trò | Tạo lúc nào | Kết thúc lúc nào | Giao tiếp với ai — qua kênh gì |
|---|---|---|---|---|---|
| main / epoll_loop | process (1 thread duy nhất) | vòng lặp epoll chính, xử lý accept + đọc/ghi client, xử lý protocol messages | khởi động server | nhận SIGTERM hoặc EOF stdin | tất cả clients thông qua sockets |

**Lý do KHÔNG dùng thread-per-client:**
- ✅ Scalable: xử lý 10K+ concurrent connections (thread có overhead stack ~8MB)
- ✅ Synchronization đơn giản: không cần locks/mutexes (single-threaded)
- ✅ Debugging dễ hơn
- epoll được thiết kế đúng cho mục đích này

## 3. Định nghĩa Protocol (bắt buộc — đây là phần quan trọng nhất)

**Chọn:** [X] Text-based dạng dòng lệnh (giống IRC), kết thúc mỗi message bằng `\n`

Liệt kê toàn bộ loại message client ↔ server:

| Chiều | Loại message | Định dạng | Ví dụ |
|---|---|---|---|
| C→S | Đăng ký | `REGISTER:username:password\n` | `REGISTER:alice:pass123\n` |
| C→S | Đăng nhập | `LOGIN:username:password\n` | `LOGIN:bob:mypass\n` |
| C→S | Gửi tin nhắn chat | `MSG:message_text\n` | `MSG:Hello everyone!\n` |
| C→S | Yêu cầu danh sách user | `USERLIST\n` | `USERLIST\n` |
| C→S | Đăng xuất | `LOGOUT\n` | `LOGOUT\n` |
| S→C | Login success | `OK:LOGIN\n` | `OK:LOGIN\n` |
| S→C | Login failed | `ERR:INVALID_CREDENTIALS\n` | `ERR:INVALID_CREDENTIALS\n` |
| S→C | Broadcast tin nhắn | `FROM:username:message\n` | `FROM:alice:Hello everyone!\n` |
| S→C | Danh sách user online | `USERS:user1,user2,user3\n` | `USERS:alice,bob,charlie\n` |
| S→C | Lỗi protocol | `ERR:INVALID_FORMAT\n` | `ERR:INVALID_FORMAT\n` |
| S→C | Lịch sử tin nhắn | `HISTORY:msg1\nHISTORY:msg2\n` | `HISTORY:alice: hello\nHISTORY:bob: hi\n` |

**Quy tắc:**
- Mỗi message kết thúc bằng `\n` (newline)
- Format: `COMMAND:PARAM1:PARAM2:...\n`
- Không hỗ trợ `:` trong message (sẽ làm sai delimiter)
- Server không phân biệt hoa/thường cho COMMAND

## 4. Xử lý Non-blocking Socket & Partial Read/Write

**Client buffer structure:**
```c
struct client {
    int fd;
    char username[64];
    int authenticated;
    
    // Input buffer (incoming data from client)
    char input_buffer[4096];
    int input_len;
    
    // Output buffer (data to send to client)
    char output_buffer[4096];
    int output_len;
    int output_pos;  // how much we've already written
};
```

**Cách xác định message đã nhận đủ:**
- Tìm `\n` trong `input_buffer`
- Nếu tìm thấy → message đã đủ, parse nó
- Nếu không tìm thấy → chờ thêm dữ liệu từ `epoll_wait()`
- Nếu buffer đầy (~4096 bytes) và không có `\n` → xem là lỗi protocol, đóng kết nối

**Cách xử lý partial write:**
```c
// When EPOLLOUT:
while (client->output_pos < client->output_len) {
    ssize_t n = write(client->fd, 
                      client->output_buffer + client->output_pos,
                      client->output_len - client->output_pos);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Can't write now, wait for next EPOLLOUT
            break;
        } else {
            // Real error, close connection
            close_client(client);
            break;
        }
    } else if (n == 0) {
        // Shouldn't happen for write(), but close if does
        close_client(client);
        break;
    } else {
        client->output_pos += n;
    }
}
if (client->output_pos >= client->output_len) {
    // All sent, disable EPOLLOUT
    client->output_pos = 0;
    client->output_len = 0;
    epoll_ctl(epfd, EPOLL_MOD, client->fd, (EPOLLIN));
}
```

## 5. Quản lý State

**Cấu trúc dữ liệu:**
- **Client list:** array `struct client clients[MAX_CLIENTS]` (fd → client_info mapping)
- **Account file:** `accounts.db` - one line per account: `username:password_hash:created_time`
- **Message history:** `messages.log` - append-only, format: `username:timestamp:message\n`

**Hash algorithm:** SHA256 (using OpenSSL `libssl-dev`)
```c
unsigned char hash[SHA256_DIGEST_LENGTH];
SHA256((unsigned char*)password, strlen(password), hash);
// Convert hash to hex string: "a1b2c3..."
```

**File locking:** 
- Khi read accounts.db: không cần lock (read-only, xảy ra thường xuyên)
- Khi write messages.log: dùng `flock(fd, LOCK_EX)` để prevent race condition
- Lock held trong 1ms (write 1 line) → không chặn server

## 6. Xử lý client ngắt kết nối đột ngột

**Phát hiện:**
```c
ssize_t n = read(client->fd, &client->input_buffer[client->input_len], 
                 sizeof(client->input_buffer) - client->input_len);
if (n < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
        // Real error (e.g., connection reset), close
        close_client(client);
    }
    // Otherwise: no data available now, try again later
} else if (n == 0) {
    // EOF: client closed connection
    close_client(client);
}
```

**Dọn dẹp tài nguyên:**
```c
void close_client(struct client *c) {
    if (c->fd < 0) return;  // Already closed
    
    // 1. Remove from epoll
    epoll_ctl(epfd, EPOLL_DEL, c->fd, NULL);
    
    // 2. Close socket
    close(c->fd);
    
    // 3. Free buffers (nếu malloc)
    // (không cần nếu dùng static array)
    
    // 4. Remove from online users
    remove_from_user_list(c->username);
    
    // 5. Broadcast disconnect message
    broadcast_message("SYSTEM", c->username + " left chat");
    
    // 6. Clear struct
    memset(c, 0, sizeof(*c));
    c->fd = -1;
}
```

## 7. Edge Case / Failure Handling dự kiến

| Tình huống | Cách xử lý dự kiến |
|---|---|
| Client gửi message không đúng protocol | ERR:INVALID_FORMAT\n + close connection |
| Client kill đột ngột giữa lúc đang gõ dở | Phát hiện read()=0 hoặc error, close_client() cleanup |
| Nhập sai password 3 lần liên tiếp | Đếm failed_attempts, sau 3 lần → close connection |
| File tài khoản/lịch sử bị khoá bởi tiến trình khác | flock() block tạm thời (timeout không cần, lock nhanh) |
| Buffer input/output đầy (4096 bytes) | Nếu input đầy mà không có \n → lỗi, đóng; nếu output đầy → keep EPOLLOUT til all sent |
| Server nhận SIGTERM | Graceful shutdown: send GOODBYE msg, close all clients, free, exit |

## 8. Bảng đối chiếu Requirement Coverage

| Requirement ID | Mô tả ngắn | Đề cập ở mục | Ghi chú |
|---|---|---|---|
| P3-M1 | Đăng ký/đăng nhập, mật khẩu hash | Mục 3, 5 | SHA256 hash |
| P3-M2 | Broadcast 1 phòng chung | Mục 3, 4 | Gửi MSG → tất cả users |
| P3-M3 | epoll, không thread-per-client | Mục 1, 2 | Single event loop |
| P3-M4 | Xử lý disconnect đột ngột | Mục 6 | read()=0 + close_client() |
| P3-M5 | Danh sách user online | Mục 3 | USERLIST command |
| P3-M6 | Xử lý sai username/password 3 lần | Mục 7 | failed_attempts counter |
| P3-M7 | Lưu & gửi lại lịch sử tin nhắn khi join | Mục 5 | messages.log append-only |

## 9. Rủi ro em thấy khó nhất trong project này

Phần khó nhất là **xử lý non-blocking I/O và partial read/write.** Nếu thiết kế sai (ví dụ: đóng connection khi `read()` return EAGAIN, hoặc không xử lý partial write) sẽ mất message hoặc làm leak file descriptor. Cần test kỹ với multiple concurrent clients.