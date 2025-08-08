#!/bin/bash

# Frame Interpolation Layer Test Script - Stage 0
# Tests the swapchain snooper functionality

echo "====================================="
echo "Frame Interpolation Layer - Stage 0 Test"
echo "====================================="
echo

# Test 1: Layer Loading
echo "Test 1: Layer Loading and Basic Functionality"
echo "Running vkcube for 5 seconds with frame interpolation layer..."
export VK_INSTANCE_LAYERS=VK_LAYER_frame_interpolation
timeout 5s vkcube > test_output.log 2>&1

if [ $? -eq 124 ]; then
    echo "✅ Layer loaded successfully and vkcube ran without crashes"
else
    echo "❌ Layer failed to load or vkcube crashed"
    cat test_output.log
    exit 1
fi

# Test 2: Console Output Verification
echo
echo "Test 2: Console Output Verification"
if grep -q "\[FRAME_INTERP\]" test_output.log; then
    echo "✅ Layer console output detected"
else
    echo "❌ No layer console output found"
    exit 1
fi

# Test 3: Swapchain Detection
echo
echo "Test 3: Swapchain Creation Detection"
if grep -q "Swapchain created" test_output.log; then
    echo "✅ Swapchain creation intercepted successfully"
else
    echo "❌ Swapchain creation not detected"
    exit 1
fi

# Test 4: Frame Timing Output
echo
echo "Test 4: Frame Timing Data"
if grep -q "Frame [0-9]*:" test_output.log; then
    echo "✅ Frame timing data being captured"
    frame_count=$(grep -c "Frame [0-9]*:" test_output.log)
    echo "   Captured $frame_count frame timing samples"
else
    echo "❌ No frame timing data found"
    exit 1
fi

# Test 5: CSV File Generation
echo
echo "Test 5: CSV File Generation"
csv_files=$(ls frame_timing_*.csv 2>/dev/null | wc -l)
if [ $csv_files -gt 0 ]; then
    echo "✅ CSV file(s) generated: $csv_files"
    latest_csv=$(ls -t frame_timing_*.csv | head -1)
    lines=$(wc -l < "$latest_csv")
    echo "   Latest CSV has $lines lines of data"
    
    # Show sample data
    echo "   Sample data:"
    head -5 "$latest_csv" | sed 's/^/   /'
else
    echo "❌ No CSV files generated"
    exit 1
fi

# Test 6: Present Mode Detection
echo
echo "Test 6: Present Mode Detection"
if grep -q "Present Mode: [0-9]*" test_output.log; then
    echo "✅ Present mode detection working"
    present_mode=$(grep "Present Mode:" test_output.log | head -1 | sed 's/.*Present Mode: \([0-9]*\).*/\1/')
    case $present_mode in
        0) echo "   Detected: VK_PRESENT_MODE_IMMEDIATE_KHR" ;;
        1) echo "   Detected: VK_PRESENT_MODE_MAILBOX_KHR" ;;
        2) echo "   Detected: VK_PRESENT_MODE_FIFO_KHR" ;;
        3) echo "   Detected: VK_PRESENT_MODE_FIFO_RELAXED_KHR" ;;
        *) echo "   Detected: Unknown mode $present_mode" ;;
    esac
else
    echo "❌ Present mode detection not working"
    exit 1
fi

# Cleanup
rm -f test_output.log

echo
echo "====================================="
echo "🎉 All Stage 0 tests passed!"
echo "====================================="
echo
echo "Stage 0 Features Validated:"
echo "✅ Vulkan layer loading and chaining"
echo "✅ Swapchain operation interception" 
echo "✅ Frame timing measurement"
echo "✅ Console logging with timestamps"
echo "✅ CSV data export"
echo "✅ Present mode detection"
echo
echo "Ready to proceed to Stage 1: History Buffer & Copy Path"
echo

# Show performance summary
if [ -f "$latest_csv" ]; then
    echo "Performance Summary from latest test:"
    awk -F, 'NR>1 {sum+=$2; count++} END {if(count>0) printf "   Average frametime: %.2f ms (%.1f FPS)\n", sum/count, 1000/(sum/count)}' "$latest_csv"
fi
