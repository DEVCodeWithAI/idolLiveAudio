# idolLiveAudio (v1.1.0)

**Lightweight, open-source Audio Plugin Host for creators, singers, and streamers.**

> Được xây dựng với sự hỗ trợ của AI (Gemini), OpenAI (ChatGPT), và FFAI Framework (Meta Llama 3)

---

## 🌐 Ngôn ngữ

- [🇺🇸 English (default)](README.md)
- [🇻🇳 Tiếng Việt](README.vi.md)

---

## 🖥️ Giao diện

![idolLiveAudio Main Interface](docs/images/screenshot_main.png)

*Giao diện nhẹ nhàng, trực quan để quản lý plugin và soundboard trong các buổi biểu diễn trực tiếp của bạn.*

---

## ✨ Điểm nổi bật trong v1.1.0: Chức năng Mini-DAW hoàn chỉnh

Phiên bản 1.1.0 biến idolLiveAudio từ một trình host plugin đơn giản thành một Mini-DAW mạnh mẽ với quy trình thu âm và phát lại hoàn chỉnh.

* **FX Chains chuyên dụng**: Mỗi track (Vocal và Music) giờ đây có 4 kênh FX Send chuyên dụng, cho phép bạn sử dụng các hiệu ứng như reverb, delay song song như trong một DAW chuyên nghiệp.
* **Player & Recorder trên từng Track**: Tải file âm thanh (nhạc nền, beat) trực tiếp vào một track để xử lý qua chuỗi plugin của track đó. Thu lại âm thanh đã qua xử lý (Post-FX) để nhanh chóng có được bản thu vocal hoặc một ý tưởng.
* **Hệ thống Project đa rãnh**:
    * **Record Project (RAW)**: Ghi âm đồng thời tín hiệu thô, chưa qua xử lý từ cả hai track Vocal và Music.
    * **Quản lý Project**: Xem, tải hoặc xóa các project đã ghi.
    * **Phát lại đồng bộ**: Tải một project và cả hai track RAW sẽ phát lại đồng bộ hoàn hảo.
* **Hệ thống Khóa an toàn (Safety Lock)**: Ngăn chặn các thay đổi vô tình trong khi biểu diễn trực tiếp bằng cách vô hiệu hóa các điều khiển cấu hình cốt lõi.

---

## 🚀 Tính năng chính

✅ Hỗ trợ Waves, Antares Auto-Tune Pro, và tất cả plugin VST3
✅ Xử lý âm thanh thời gian thực với độ trễ thấp
✅ Quản lý chuỗi plugin linh hoạt trên mỗi track
✅ FX Chains chuyên dụng để xử lý song song (Reverb, Delay,...)
✅ Player & Recorder tích hợp trên mỗi track (Post-FX)
✅ Hệ thống Project đa rãnh để ghi âm thanh RAW
✅ Khóa an toàn để chống thay đổi cấu hình ngoài ý muốn
✅ Soundboard tích hợp để phát âm thanh nhanh
✅ Giao diện đơn giản, thân thiện với người dùng
✅ Phát triển với JUCE (C++20)
✅ Mã nguồn mở theo giấy phép GPLv3

---

## 🗺️ Lộ trình Phát triển (Roadmap)

### Hiện tại: Phiên bản Solo (v1.1.0)

Phiên bản hiện tại được tối ưu hóa cho các nghệ sĩ solo, streamer và người sáng tạo nội dung biểu diễn một mình, cung cấp đầy đủ công cụ để xử lý một track vocal và một track nhạc nền một cách chuyên nghiệp.

### Tương lai: Phiên bản Pro

Một phiên bản **idolLiveAudio Pro** đang được lên kế hoạch với các tính năng nâng cao dành cho người dùng chuyên nghiệp:

* **Không giới hạn Track**: Hỗ trợ không giới hạn số lượng track Vocal và Music.
* **Tích hợp Chuyên sâu**: Tương thích sâu hơn với các plugin từ các nhà cung cấp lớn (Vendor) để đảm bảo hiệu suất và độ ổn định tối đa.
* **Phím tắt Toàn cục (Global Hotkey)**: Kích hoạt các hành động trong ứng dụng ngay cả khi cửa sổ không được focus, được hỗ trợ bởi chứng chỉ ký số an toàn.
* **Quản lý bản quyền**: Tích hợp giao thức đăng nhập vào máy chủ để quản lý giấy phép sử dụng.

> **Giá dự kiến cho bản Pro:** **$49 USD** cho giấy phép sử dụng vĩnh viễn.

---

## 📦 Cài đặt

**Lựa chọn 1: Tải bản dựng sẵn (Khuyên dùng)**

* Truy cập mục [**Releases**](https://github.com/DEVCodeWithAI/idolLiveAudio/releases) để xem phiên bản mới nhất.
* Tải về gói `.zip` phù hợp.
* Giải nén và chạy ứng dụng.

**Lựa chọn 2: Biên dịch từ mã nguồn**

* Yêu cầu trình biên dịch tương thích C++20.
* Yêu cầu [JUCE Framework](https://juce.com).
* Mở file `idolLiveAudio.jucer` bằng Projucer.
* Xuất dự án sang IDE bạn muốn (Visual Studio, Xcode, v.v.).
* Build và chạy.

---

## ⚠️ Lưu ý quan trọng cho người dùng Waves

Nếu bạn có một bộ sưu tập plugin Waves lớn (ví dụ: Waves Ultimate), quá trình quét plugin lần đầu có thể mất nhiều thời gian. **Đây là điều bình thường!** Vui lòng không đóng ứng dụng trong khi quét.

* **Thời gian quét ước tính:**
    * Bộ plugin nhỏ: Vài giây đến 2 phút.
    * Bộ plugin lớn (Waves Ultimate): Có thể lên tới 10-15 phút.

✅ Quá trình quét chỉ diễn ra một lần. Sau khi hoàn tất, idolLiveAudio sẽ lưu kết quả để khởi động nhanh hơn trong các lần tiếp theo.

---

## 🎧 Cấu hình Âm thanh đề xuất

Để có trải nghiệm tốt nhất, bạn nên sử dụng sound card rời có driver ASIO độ trễ thấp.

**Nếu không có sound card chuyên nghiệp, chúng tôi đặc biệt khuyên bạn nên cài đặt các công cụ miễn phí sau:**

* [**VB-Cable**](https://vb-audio.com/Cable/) – Cáp âm thanh ảo.
* [**ASIO4ALL**](https://www.asio4all.org/) – Driver ASIO phổ thông cho âm thanh độ trễ thấp.
* [**Voicemeeter Banana**](https://vb-audio.com/Voicemeeter/banana.htm) – Bàn trộn và định tuyến âm thanh ảo.

---

## 💡 Đóng góp

Chúng tôi hoan nghênh sự đóng góp của cộng đồng! Bạn có thể giúp bằng cách:

* Báo cáo lỗi.
* Đề xuất tính năng.
* Gửi Pull Request.
* Chia sẻ các preset và cấu hình.

Xem chi tiết tại [CONTRIBUTING.md](CONTRIBUTING.md).

---

## ☕ Ủng hộ Dự án

Dự án này được tự tài trợ. Nếu bạn thấy idolLiveAudio hữu ích, hãy cân nhắc mời tôi một ly cà phê:

👉 [**https://buymeacoffee.com/devcodewithai**](https://buymeacoffee.com/devcodewithai)

Sự ủng hộ của bạn giúp trang trải thời gian phát triển và các kế hoạch trong tương lai như website, diễn đàn cộng đồng, và các tính năng nâng cao.