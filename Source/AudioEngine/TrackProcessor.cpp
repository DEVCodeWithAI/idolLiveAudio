/*
  ==============================================================================

    TrackProcessor.cpp
    (Đã được tái cấu trúc để kế thừa từ ProcessorBase)

  ==============================================================================
*/

#include "TrackProcessor.h"

//==============================================================================
// <<< MODIFIED >>> Constructor now passes the ID to the base class
TrackProcessor::TrackProcessor(const juce::Identifier& id)
    : ProcessorBase(id)
{
    // Constructor của lớp cơ sở (ProcessorBase) sẽ tự động được gọi,
    // khởi tạo gain và các thành viên khác.
}

TrackProcessor::~TrackProcessor()
{
}

// Tất cả các hàm như prepare, process, reset, setGain, setMuted, etc.
// giờ đây được kế thừa trực tiếp từ ProcessorBase.
// Nếu chúng ta cần hành vi đặc biệt cho TrackProcessor trong tương lai,
// chúng ta có thể override các hàm đó ở đây.