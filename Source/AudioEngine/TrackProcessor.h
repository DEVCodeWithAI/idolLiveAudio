/*
  ==============================================================================

    TrackProcessor.h
    (Đã được tái cấu trúc để kế thừa từ ProcessorBase)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "ProcessorBase.h" // Bao gồm lớp cơ sở mới

//==============================================================================
/**
    Lớp TrackProcessor giờ đây kế thừa từ ProcessorBase,
    tận dụng tất cả các logic chung về chuỗi plugin, gain và mute.
    Lớp này hiện tại không cần thêm logic cụ thể nào.
*/
class TrackProcessor : public ProcessorBase
{
public:
    // <<< MODIFIED >>> Constructor now accepts an Identifier
    TrackProcessor(const juce::Identifier& id);
    ~TrackProcessor() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackProcessor)
};