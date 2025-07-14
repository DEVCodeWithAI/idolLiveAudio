# idolLiveAudio (v1.3.1)

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

## ✨ Điểm nổi bật trong v1.3.1

Phiên bản 1.3.1 tập trung cải tiến trải nghiệm cốt lõi, nhấn mạnh vào sự ổn định và tốc độ cho người biểu diễn trực tiếp.

* **⚡️ Chuyển đổi Preset Tức thì (Hot-Swap)**: Thay đổi giữa các preset có thông số plugin khác nhau một cách tức thời, không bị ngắt quãng hay gián đoạn âm thanh. Hoàn hảo cho việc chuyển đổi hiệu ứng giọng hát giữa các bài hát khi đang livestream.
* **🧠 Nhận diện Thay đổi Thông minh**: Ứng dụng giờ đây đủ thông minh để bỏ qua các thay đổi từ những plugin không cần chỉnh sửa (như plugin dò tone), ngăn chặn các thông báo "Bạn có muốn lưu không?" không cần thiết.
* **🛡️ Quản lý Trạng thái Vững chắc**: Logic lưu và tải preset đã được đại tu để bao gồm cả trạng thái khóa giao diện, đảm bảo cấu hình của bạn được khôi phục chính xác như lúc bạn rời đi.
* **🐛 Sửa lỗi & Ổn định**: Các cải tiến chung về việc host plugin và quản lý preset để đảm bảo trải nghiệm mượt mà, đáng tin cậy hơn.

---

## 🚀 Tính năng chính

✅ **Chuyển đổi Preset "Nóng" Tức thì** cho buổi biểu diễn liền mạch
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

## 💡 Mẹo chuyên nghiệp: Cài đặt Plugin nhận diện Tone

Để có trải nghiệm tốt nhất, bạn nên đặt các plugin nhận diện tone (ví dụ: **Antares Auto-Key**, **Waves Key Detector**...) vào **ô plugin đầu tiên của track Music**.

Ứng dụng đã được tối ưu hóa đặc biệt để bỏ qua các thay đổi trạng thái từ ô này. Điều này giúp ứng dụng không hỏi bạn lưu preset sau khi plugin tự động nhận diện một tone mới, đảm bảo một quy trình làm việc thực sự liền mạch.

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