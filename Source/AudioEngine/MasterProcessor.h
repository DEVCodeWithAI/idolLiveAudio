/*
  ==============================================================================

    MasterProcessor.h
    (Đã được tái cấu trúc để kế thừa từ ProcessorBase)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "ProcessorBase.h" // Bao gồm lớp cơ sở mới

//==============================================================================
/**
    Lớp MasterProcessor giờ đây kế thừa từ ProcessorBase,
    tận dụng tất cả các logic chung về chuỗi plugin, gain và mute.
    Điều này giúp chúng ta dễ dàng thêm plugin vào kênh tổng trong tương lai.
*/
class MasterProcessor : public ProcessorBase
{
public:
    // <<< MODIFIED >>> Constructor now accepts an Identifier
    MasterProcessor(const juce::Identifier& id);
    ~MasterProcessor() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterProcessor)
};