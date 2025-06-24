/*
  ==============================================================================

    MasterProcessor.cpp
    (Đã được tái cấu trúc để kế thừa từ ProcessorBase)

  ==============================================================================
*/

#include "MasterProcessor.h"

//==============================================================================
// <<< MODIFIED >>> Constructor now passes the ID to the base class
MasterProcessor::MasterProcessor(const juce::Identifier& id)
    : ProcessorBase(id)
{
    // Constructor của lớp cơ sở (ProcessorBase) sẽ tự động được gọi,
    // khởi tạo gain và các thành viên khác.
}

MasterProcessor::~MasterProcessor()
{
}

// Tất cả các hàm xử lý âm thanh và điều khiển giờ đây được kế thừa
// từ lớp ProcessorBase. Mã nguồn của MasterProcessor trở nên rất gọn gàng.