# PROJECT 3: Multi-user CLI Chat Server/Client

**Thời lượng:** 4 tuần | **Độ khó:** Trung bình | **Nộp bài:** GitHub PR (hệ thống chấm điểm tự động `devlinux-reviewer`, ngưỡng ≥80 Pass / 60–79 Pass with revisions / <60 Fail)

**Thiết bị:** chỉ cần máy Linux (VM/container/máy thật), không cần phần cứng.

**Mô phỏng sản phẩm thật:** hệ thống chat dạng IRC/Slack rút gọn.

**Kiến thức áp dụng:** Socket TCP, epoll, File I/O (lưu tài khoản), băm mật khẩu, quản lý phiên đồng thời.

> Quy ước ID requirement (`P3-M1`, `P3-N1`...) dùng chung giữa đề bài này, `DESIGN.md` học viên nộp, và rubric chấm điểm — xem thêm quy định chung ở `00_Chung/QUY_DINH_CHUNG.md`.

---

## Problem Statement
Xây dựng server chat CLI đa người dùng qua TCP, hỗ trợ đăng ký/đăng nhập và chat chung trong **1 phòng broadcast duy nhất**, toàn bộ xử lý đồng thời nhiều client bằng **epoll** (không thread-per-client).

## Functional Requirements

**Must-have:**
- **P3-M1.** Đăng ký/đăng nhập tài khoản, mật khẩu lưu ở dạng **hash** (không lưu plaintext), lưu trong file.
- **P3-M2.** Sau khi đăng nhập, mọi tin nhắn gửi lên đều **broadcast tới toàn bộ user đang online** (1 phòng chung duy nhất, không cần tạo/quản lý nhiều phòng).
- **P3-M3.** Server dùng epoll để xử lý toàn bộ kết nối trên 1 (hoặc ít) thread, không được dùng 1 thread/1 client.
- **P3-M4.** Xử lý client ngắt kết nối đột ngột (crash/kill) mà không làm sập server hay rò rỉ tài nguyên.
- **P3-M5.** Danh sách user đang online.
- **P3-M6.** Nhập sai username/password: server trả về lỗi rõ ràng, **không** cho biết cụ thể là sai username hay sai password (tránh lộ thông tin tài khoản nào tồn tại), client cho phép nhập lại tối đa 3 lần trước khi tự ngắt kết nối.
- **P3-M7.** Server lưu **toàn bộ lịch sử tin nhắn** ra file; khi một user đăng nhập/join vào phòng chung, server gửi lại **toàn bộ lịch sử tin nhắn** đã có từ trước cho client đó xem trước khi vào chat realtime.

**Nice-to-have:**
- **P3-N1.** Nhiều phòng chat (tạo/tham gia/rời phòng), broadcast chỉ trong phạm vi phòng thay vì toàn server.
- **P3-N2.** Tin nhắn riêng (private message) giữa 2 user.
- **P3-N3.** Giới hạn số lượng tin nhắn lịch sử hiển thị (ví dụ chỉ N tin gần nhất) thay vì toàn bộ, kèm lệnh `/history` xem thêm.
- **P3-N4.** Lệnh admin (kick user).
- **P3-N5.** Giới hạn rate gửi tin nhắn để chống spam.

## Design Hints (lưu ý kỹ thuật, tránh bẫy thường gặp)
- Thiết kế protocol đơn giản dạng text-based (giống IRC) hoặc JSON-line, phải document rõ ràng trong README.
- Dùng buffer riêng cho từng client để xử lý trường hợp đọc/ghi không trọn vẹn (partial read/write) qua non-blocking socket.
- Đây là project cố ý có bug đồng bộ hoá (nếu dùng thêm thread phụ) để sinh viên phải debug bằng Helgrind/GDB.
- Lịch sử tin nhắn nên ghi ra file (append-only) mỗi khi có tin nhắn mới, và đọc lại toàn bộ file này để gửi cho client vừa đăng nhập — chú ý dùng `flock`/`fcntl` nếu có khả năng nhiều nơi ghi đồng thời vào file log.
- Gửi lịch sử tin nhắn cho client mới nên tách biệt rõ với luồng broadcast realtime để tránh lẫn lộn thứ tự (client cần nhận đủ và đúng thứ tự lịch sử trước khi nhận tin nhắn mới).

## Gợi ý thiết kế cho học viên

**Gợi ý kiến trúc tổng thể:** đây là bài chỉ cần **1 vòng lặp epoll duy nhất** ở trung tâm, không cần nhiều thread. Hình dung server gồm:
- 1 mảng/map `clients[fd] → struct client_info` (username, buffer đọc dở, trạng thái đăng nhập, số lần nhập sai...).
- Vòng lặp `epoll_wait` chính xử lý 3 loại sự kiện: (1) fd lắng nghe có kết nối mới → `accept`, thêm vào epoll; (2) fd client có dữ liệu để đọc → đọc vào buffer riêng của client đó, kiểm tra đã đủ 1 message chưa (theo delimiter đã chọn); (3) phát hiện lỗi/đóng kết nối → dọn dẹp.
- Khi 1 client gửi xong 1 message hợp lệ, code **không gửi trả ngay trong vòng lặp đọc** mà đẩy vào 1 hàm `broadcast_to_all()` duyệt qua toàn bộ `clients[]` còn lại và gửi.

### Chi tiết kỹ thuật — epoll & Non-blocking Socket

**Khung vòng lặp chính gợi ý:**
```c
int epfd = epoll_create1(0);
// set listen_fd non-blocking, add vào epoll với EPOLLIN
struct epoll_event ev = { .events = EPOLLIN, .data.fd = listen_fd };
epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

while (1) {
    int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
    for (int i = 0; i < n; i++) {
        if (events[i].data.fd == listen_fd) {
            int client_fd = accept(listen_fd, ...);
            set_nonblocking(client_fd);
            struct epoll_event cev = { .events = EPOLLIN, .data.fd = client_fd };
            epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &cev);
            init_client_buffer(client_fd);
        } else {
            handle_client_data(events[i].data.fd);  // đọc, parse, xử lý hoặc dọn dẹp nếu disconnect
        }
    }
}
```
**Nên dùng level-triggered (mặc định của epoll, không cần `EPOLLET`)** cho bài này — đơn giản hơn nhiều so với edge-triggered (không cần vòng lặp `read` tới khi `EAGAIN`), và bài không yêu cầu hiệu năng cực cao nên không cần tối ưu bằng edge-triggered.

**Xử lý buffer & framing message (quan trọng nhất khi dùng non-blocking socket):**
- Mỗi client cần 1 buffer riêng (ví dụ `char buf[4096]` + con trỏ `len` đánh dấu đã có bao nhiêu byte) vì 1 lần `recv()` có thể nhận **chưa đủ** 1 message, hoặc nhận **nhiều hơn** 1 message cùng lúc.
- Nếu chọn protocol dạng text kết thúc bằng `\n` (khuyến khích, dễ debug bằng `nc`/telnet): sau mỗi lần `recv()` thêm dữ liệu vào buffer, quét tìm `\n` trong buffer — tìm thấy thì tách ra xử lý 1 message, phần còn lại giữ lại buffer cho lần đọc sau; lặp lại quét vì có thể có nhiều `\n` trong 1 lần nhận.
- `recv()` trả về `0` → client đã đóng kết nối chủ động; trả về `-1` với `errno == EAGAIN`/`EWOULDBLOCK` → chưa có dữ liệu mới (bình thường với non-blocking, không phải lỗi); trả về `-1` với `errno` khác → lỗi thật, nên đóng kết nối.

### Chi tiết kỹ thuật — Hash password

Dùng `crypt()` (glibc, hỗ trợ SHA-256/512 qua tiền tố `$5$`/`$6$`) là đủ cho bài, không cần thư viện ngoài:
```c
#include <crypt.h>
char *hashed = crypt(password, "$6$somesalt$");   // lưu chuỗi `hashed` vào file, không lưu password gốc
// khi đăng nhập: so sánh crypt(input_password, hashed) == hashed
```
Nhớ link `-lcrypt` khi biên dịch.

**Gợi ý thứ tự nên làm:**
1. **Tuần 1:** viết xong khung epoll server cơ bản — chấp nhận nhiều kết nối, đọc/ghi echo đơn giản (chưa cần auth/broadcast gì), test bằng `nc` trước khi viết client riêng.
2. **Tuần 2:** thêm đăng ký/đăng nhập + hash password (P3-M1, M6), rồi broadcast thật (P3-M2). Đây là lúc nên tự thiết kế xong protocol (mục 2 trong DESIGN.md) trước khi code tiếp.
3. **Tuần 3:** thêm lưu & gửi lịch sử khi join (P3-M7), xử lý disconnect đột ngột sạch sẽ (P3-M4, test bằng cách `kill -9` client liên tục để soi leak fd qua `lsof`).
4. **Tuần 4:** hoàn thiện `/who` (P3-M5), viết client CLI theo đúng hành vi mẫu, viết README định nghĩa protocol, chạy đủ Test Case.

**Gợi ý test thủ công khi code:** dùng `nc localhost <port>` để giả lập client thô khi debug protocol, không cần chờ viết xong client CLI hoàn chỉnh mới test được server.

## Client CLI Behavior (mẫu tham khảo)

**Lúc mới mở client:**
```
=== DevLinux Chat Client ===
Server: 127.0.0.1:9000
Đăng nhập hay Đăng ký? [login/register]: login
Username: viet
Password: ********
```

**Nhập sai username hoặc password:**
```
[!] Sai username hoặc password. Vui lòng thử lại (2 lần thử còn lại).
Username: viet
Password: ********
```
Sau 3 lần nhập sai liên tiếp, client tự ngắt kết nối và in ra:
```
[!] Đã nhập sai quá số lần cho phép. Kết nối đã bị đóng.
```

**Sau khi đăng nhập thành công — server gửi lại toàn bộ lịch sử tin nhắn trước, rồi vào màn hình chat chính:**
```
[Connected as viet — 3 users online]
--- Lịch sử tin nhắn ---
[09:10:02] minh: có ai đang test server không?
[09:15:44] lan: em đang test đây anh
[10:32:01] minh: chào mọi người
[10:32:15] lan: hi hi
--------------------------------------
[10:32:40] *** khoa has joined ***
> _
```
Người dùng gõ tin nhắn rồi Enter để gửi broadcast. Ngoài ra hỗ trợ một số lệnh bắt đầu bằng `/` để không lẫn với nội dung chat thường:
```
/who        -> liệt kê user đang online
/quit       -> thoát chương trình
/help       -> xem danh sách lệnh
```

**Khi có tin nhắn mới tới trong lúc đang gõ dở**, tin nhắn phải in phía trên dòng nhập hiện tại, không được đè lên hoặc xoá mất nội dung người dùng đang gõ dở.

**Khi mất kết nối server:**
```
[!] Mất kết nối tới server. Đang thử kết nối lại...
```

## Expected Output
- Demo nhiều terminal client kết nối đồng thời, đăng nhập, gửi tin nhắn thấy broadcast tới toàn bộ client khác, một client bị kill đột ngột không ảnh hưởng client khác.
- Demo riêng luồng nhập sai username/password 3 lần liên tiếp, thấy client tự ngắt kết nối đúng như mô tả.

## Submission
- GitHub PR, tuân thủ đúng **Cấu trúc bàn giao chuẩn** bên dưới. Định nghĩa protocol phải nêu rõ trong README. Kết quả tự chạy Test Case đính kèm trong `docs/test_report.md`.

## Cấu trúc bàn giao chuẩn — Project 3
```
<student-repo>/
├── DESIGN.md
├── README.md                      # bắt buộc có mục định nghĩa Protocol
├── src/
│   ├── server/
│   │   ├── main.c                 # epoll loop chính (P3-M3)
│   │   ├── auth.c                 # đăng ký/đăng nhập, hash password (P3-M1, P3-M6)
│   │   ├── broadcast.c            # broadcast tin nhắn (P3-M2)
│   │   └── history.c              # lưu & gửi lịch sử tin nhắn khi join (P3-M7)
│   └── client/
│       └── main.c                 # client CLI (theo đúng Client CLI Behavior đã định nghĩa)
└── docs/
    └── test_report.md             # kết quả tự chạy TESTCASES_P3_student.md (bắt buộc, kèm log terminal)
```
