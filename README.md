# Hệ Thống Quản Lý Giao Thông Đường Phố

## Mô tả
Ứng dụng web cho phép:
- Tìm đường ngắn nhất giữa hai điểm trên bản đồ (nhiều thuật toán).
- Cấm đường (1 chiều, 2 chiều).
- Cấm vùng (đa giác).
- Đánh dấu tắc đường (tăng thời gian đi qua đoạn đường).
- Khôi phục các thao tác vừa thực hiện.
- Giao diện trực quan với bản đồ Leaflet.

## Hướng dẫn cài đặt

1. **Yêu cầu:**
   - Python 3.8+
   - Các thư viện: `Flask`, `osmnx`, `networkx`, `shutil`, `jquery`, `leaflet`
   - File bản đồ: `congvi_badinh_hanoi_graph.graphml` (đặt cùng thư mục với mã nguồn)

2. **Cài đặt thư viện:**
   ```bash
   pip install flask osmnx networkx
   ```

3. **Chạy ứng dụng:**
   ```bash
   python Deploy.py
   ```
   - Truy cập giao diện người dùng: [http://localhost:8000/](http://localhost:8000/)
   - Đăng nhập admin: [http://localhost:8000/admin](http://localhost:8000/admin)
     - Tài khoản mặc định: `admin` / `admin123`

## Hướng dẫn sử dụng

### 1. Tìm đường
- Chọn 2 điểm bất kỳ trên bản đồ.
- Chọn thuật toán tìm đường.
- Đường đi ngắn nhất sẽ được vẽ màu đỏ.

### 2. Cấm đường
- Vào trang admin.
- Chọn "Cấm đường" → chọn chiều cấm → bật chế độ cấm đường.
- Click 2 điểm trên bản đồ để cấm đoạn đường đó.
- Có thể khôi phục đoạn vừa cấm.

### 3. Cấm vùng
- Chọn "Cấm vùng" → bật chế độ cấm vùng.
- Click nhiều điểm để tạo vùng đa giác.
- Nhấn "Gửi vùng cấm" để xác nhận.
- Có thể khôi phục vùng vừa cấm.

### 4. Tắc đường
- Chọn "Tắc đường" → nhập hệ số tắc → bật chế độ tắc đường.
- Click 2 điểm để đánh dấu đoạn tắc (đoạn này sẽ có thời gian đi lâu hơn, vẽ màu đen gạch đứt).
- Có thể khôi phục đoạn tắc vừa đánh dấu.

## Ghi chú
- Các thao tác cấm/tắc chỉ có hiệu lực trong phiên đăng nhập hiện tại.
- Khi đăng nhập lại hoặc khởi động lại server, bản đồ sẽ trở về trạng thái ban đầu.
- Không cần cài thêm thư viện ngoài trừ các thư viện Python đã liệt kê.

## Thư mục chính

- `Deploy.py`         : Backend Flask
- `shortest_path.py`  : Các thuật toán tìm đường
- `templates/`        : Giao diện HTML (index, admin_panel)
- `congvi_badinh_hanoi_graph.graphml` : File bản đồ

---

**Mọi thắc mắc vui lòng liên hệ chủ dự án.**