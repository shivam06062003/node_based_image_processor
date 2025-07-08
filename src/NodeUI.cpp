﻿#include "NodeUI.h"
#include <iostream>
#include "OutputNode.h"
#include <imgui_internal.h>
#include "filters/BrightnessContrastNode.h"
#include "filters/BlurNode.h"
#include "filters/ThresholdNode.h"
#include "filters/EdgeDetectionNode.h"
#include "filters/BlendNode.h"
#include "filters/NoiseNode.h" 
#include "NodeGraph.h"
#include "filters/ConvolutionNode.h"
#include "filters/ColorChannelSplitterNode.h"
#include "NodeFactory.h" // needed for CreateNodeByName
#include "tinyfiledialogs/tinyfiledialogs.h"
PendingConnection pendingConnection;
extern NodeGraph graph;
extern std::vector<BaseNode*> nodes;  // 👈 Add this at the top if not already
void NodeUIManager::RenderNode(BaseNode& node) {
    int id = node.id;
    std::string name = node.name;
    NodeUIState& state = nodeStates[id];
    ImVec2 scrollOffset(ImGui::GetScrollX(), ImGui::GetScrollY());
    

    if (state.position.x == 0 && state.position.y == 0)
        state.position = ImVec2(100 + id * 180, 250);

    ImVec2 nodeSize = ImVec2(200, 180);
    ImVec2 start = ImVec2(state.position.x - scrollOffset.x, state.position.y - scrollOffset.y);
    ImVec2 end = ImVec2(start.x + nodeSize.x, start.y + nodeSize.y);

    ImGui::SetCursorScreenPos(start);
    ImGui::PushID(id);
    ImGui::BeginGroup();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(start, end, IM_COL32(40, 40, 50, 255), 6.0f);
    drawList->AddRect(start, end, IM_COL32(255, 255, 255, 255), 6.0f);
    
    drawList->AddText(ImVec2(start.x + 10, start.y + 10), IM_COL32(255, 255, 255, 255), name.c_str());

    // ... keep existing UI logic as-is ...
    // Output preview (optional)
    if (OutputNode* outNode = dynamic_cast<OutputNode*>(&node)) {
        if (outNode->textureID) {
            ImTextureID texID = (ImTextureID)(intptr_t)outNode->textureID;
            ImGui::SetCursorScreenPos(ImVec2(start.x + 10, start.y + 30));
            ImGui::Image(texID, ImVec2(180, 100));
        }
        ImGui::SetCursorScreenPos(ImVec2(start.x + 10, start.y + 135));
        if (ImGui::Button(("Save##" + std::to_string(id)).c_str())) {
            const char* filters[] = { "*.png", "*.jpg", "*.bmp" };
            const char* file = tinyfd_saveFileDialog("Save Image", "output.png", 3, filters, nullptr);
            if (file && strlen(file) > 0) {
                if (!outNode->SaveImage(file)) {
                    std::cerr << "[OutputNode] Failed to save image.\n";
                }
            }
        }
    }
    if (ImageInputNode* inNode = dynamic_cast<ImageInputNode*>(&node)) {
        if (inNode->textureID) {
            ImTextureID texID = (ImTextureID)(intptr_t)inNode->textureID;
            ImGui::SetCursorScreenPos(ImVec2(start.x + 10, start.y + 30));
            ImGui::Image(texID, ImVec2(180, 100));
        }
        ImGui::SetCursorScreenPos(ImVec2(start.x + 10, start.y + 135));  // Position below image
        if (ImGui::Button(("Load##" + std::to_string(id)).c_str())) {
            const char* filters[] = { "*.jpg", "*.png", "*.bmp" };
            const char* file = tinyfd_openFileDialog("Select Image", "", 3, filters, NULL, 0);
            if (file && strlen(file) > 0) {
                inNode->SetImagePath(file);
                graph.ProcessAll();
            }
        }
    }
    if (ColorChannelSplitterNode* splitter = dynamic_cast<ColorChannelSplitterNode*>(&node)) {
        float controlWidth = nodeSize.x - 20;
        ImGui::PushItemWidth(controlWidth);
        ImGui::SetCursorScreenPos(ImVec2(start.x + 10, start.y + 30));
        const char* items[] = { "Red", "Green", "Blue", "Alpha" };
        int selected = static_cast<int>(splitter->selectedOutput);
        if (ImGui::Combo(("##channel_" + std::to_string(id)).c_str(), &selected, items, IM_ARRAYSIZE(items))) {
            splitter->selectedOutput = static_cast<ChannelOutput>(selected);
        }
        ImGui::PopItemWidth();
    }

    if (BrightnessContrastNode* bc = dynamic_cast<BrightnessContrastNode*>(&node)) {
        ImGui::SetCursorScreenPos(ImVec2(start.x + 10, start.y + 30));
        

        float controlWidth = nodeSize.x - 20;
        ImGui::PushItemWidth(controlWidth);
        ImGui::Text("Brightness/Contrast Settings");

        bool updated = false;

        updated |= ImGui::SliderFloat("Brightness", &bc->brightness, -100.0f, 100.0f);
        updated |= ImGui::SliderFloat("Contrast", &bc->contrast, 0.0f, 3.0f);

        if (ImGui::Button("Reset Brightness")) {
            bc->brightness = 0.0f;
            updated = true;
        }

        if (ImGui::Button("Reset Contrast")) {
            bc->contrast = 1.0f;
            updated = true;
        }

        if (updated) {
            bc->Process();
            graph.ProcessAll();
        }
        ImGui::PopItemWidth();
    }
    if (BlurNode* blur = dynamic_cast<BlurNode*>(&node)) {
        ImGui::SetCursorScreenPos(ImVec2(start.x + 10, start.y + 30));
        ImGui::Text("Radius");

        float sliderWidth = nodeSize.x - 5;  // total padding: 10 on each side
        ImGui::PushItemWidth(sliderWidth);
        if (ImGui::SliderInt(("##blur_" + std::to_string(id)).c_str(), &blur->radius, 1, 20)) {
            blur->Process();
            graph.ProcessAll();
        }
        ImGui::PopItemWidth();
    }

    if (ThresholdNode* th = dynamic_cast<ThresholdNode*>(&node)) {
        ImGui::SetCursorScreenPos(ImVec2(start.x + 10, start.y + 30));
        ImGui::Text("Threshold");
        float sliderWidth = nodeSize.x - 20;  // total padding: 10 on each side
        ImGui::PushItemWidth(sliderWidth);
        if (ImGui::SliderInt(("##threshold_" + std::to_string(id)).c_str(), &th->thresholdValue, 0, 255)) {
            th->Process();
            graph.ProcessAll();
        }
        ImGui::PopItemWidth();
    }
    if (BlendNode* blend = dynamic_cast<BlendNode*>(&node)) {
        ImGui::SetCursorScreenPos(ImVec2(start.x + 10, start.y + 30));
        float controlWidth = nodeSize.x - 20;
        ImGui::PushItemWidth(controlWidth);
        const char* modes[] = { "Normal", "Multiply", "Screen", "Overlay", "Difference" };
        int current = static_cast<int>(blend->mode);
        if (ImGui::Combo(("Mode##" + std::to_string(id)).c_str(), &current, modes, 5)) {
            blend->mode = static_cast<BlendMode>(current);
            blend->Process();
            graph.ProcessAll();
        }

        if (ImGui::SliderFloat(("Opacity##" + std::to_string(id)).c_str(), &blend->opacity, 0.0f, 1.0f)) {
            blend->Process();
            graph.ProcessAll();
        }
        ImGui::PopItemWidth();
    }
    if (NoiseNode* noise = dynamic_cast<NoiseNode*>(&node)) {
        float controlWidth = nodeSize.x - 20;
        ImGui::PushItemWidth(controlWidth);
        if (noise->textureID) {
            ImGui::SetCursorScreenPos(ImVec2(start.x + 10, start.y + 30));
            ImTextureID tex = (ImTextureID)(intptr_t)noise->textureID;
            ImGui::Image(tex, ImVec2(180, 100));
        }

        // Noise sliders...
        ImGui::Text("Noise Parameters:");
        ImGui::SliderInt("Width", &noise->width, 64, 1024);
        ImGui::SliderInt("Height", &noise->height, 64, 1024);
        ImGui::SliderFloat("Scale", &noise->scale, 1.0f, 100.0f);
        ImGui::SliderInt("Seed", &noise->seed, 0, 999);

        if (ImGui::Button("Generate")) {
            noise->Process();
            graph.ProcessAll();
        }
        ImGui::PopItemWidth();
    }

    if (EdgeDetectionNode* edge = dynamic_cast<EdgeDetectionNode*>(&node)) {
        ImGui::SetCursorScreenPos(ImVec2(start.x + 10, start.y + 30));
        float controlWidth = nodeSize.x - 20;
        ImGui::PushItemWidth(controlWidth);
        const char* modes[] = { "Sobel", "Canny" };
        int current = (edge->mode == EdgeMode::Sobel) ? 0 : 1;
        if (ImGui::Combo(("##edgemode_" + std::to_string(id)).c_str(), &current, modes, 2)) {
            edge->mode = (current == 0) ? EdgeMode::Sobel : EdgeMode::Canny;
            edge->Process();
            graph.ProcessAll();
        }

        if (edge->mode == EdgeMode::Sobel) {
            if (ImGui::SliderInt(("Kernel##" + std::to_string(id)).c_str(), &edge->kernelSize, 1, 7)) {
                if (edge->kernelSize % 2 == 0) edge->kernelSize += 1;  // kernel must be odd
                edge->Process();
                graph.ProcessAll();
            }
        }
        else {
            if (ImGui::SliderInt(("Thresh1##" + std::to_string(id)).c_str(), &edge->cannyThreshold1, 0, 255)) {
                edge->Process();
                graph.ProcessAll();
            }
            if (ImGui::SliderInt(("Thresh2##" + std::to_string(id)).c_str(), &edge->cannyThreshold2, 0, 255)) {
                edge->Process();
                graph.ProcessAll();
            }
        }
        ImGui::PopItemWidth();
    }
    if (ConvolutionNode* conv = dynamic_cast<ConvolutionNode*>(&node)) {
        ImGui::SetCursorScreenPos(ImVec2(start.x + 10, start.y + 30));
        float controlWidth = nodeSize.x - 20;
        ImGui::PushItemWidth(controlWidth);
        static const char* presets[] = { "Sharpen", "Edge", "Emboss" };
        static int currentPreset = 0;
        if (ImGui::Combo(("Preset##" + std::to_string(id)).c_str(), &currentPreset, presets, IM_ARRAYSIZE(presets))) {
            conv->SetPreset(presets[currentPreset]);
            conv->Process();
            graph.ProcessAll();
        }

        ImGui::Text("Kernel:");
        for (int i = 0; i < conv->kernel.size(); ++i) {
            if (i % conv->kernelSize != 0)
                ImGui::SameLine();

            ImGui::PushItemWidth(40);
            if (ImGui::InputFloat(("##k" + std::to_string(id) + "_" + std::to_string(i)).c_str(), &conv->kernel[i], 0.1f, 1.0f, "%.1f")) {
                conv->Process();
                graph.ProcessAll();
            }
            ImGui::PopItemWidth();
        }
        ImGui::PopItemWidth();
    }




    ImGui::SetCursorScreenPos(start);
    ImGui::InvisibleButton("node_drag", nodeSize);

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        state.position.x += ImGui::GetIO().MouseDelta.x;
        state.position.y += ImGui::GetIO().MouseDelta.y;
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        selectedNodeID = id;  // Select this node
    }
    if (id == selectedNodeID) {
        drawList->AddRect(start, end, IM_COL32(0, 255, 0, 255), 6.0f, 0, 3.0f);
    }
    ImGui::EndGroup();
    ImGui::PopID();
}

void NodeUIManager::RenderConnections() {
    ImVec2 scrollOffset(ImGui::GetScrollX(), ImGui::GetScrollY());
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Draw all existing connections
    for (size_t i = 0; i < connections.size(); ++i) {
        const auto& conn = connections[i];
        ImVec2 fromPos = nodeStates[conn.fromID].position;
        ImVec2 toPos = nodeStates[conn.toID].position;

        fromPos = ImVec2(fromPos.x - scrollOffset.x + 200, fromPos.y - scrollOffset.y + 70);
        toPos = ImVec2(toPos.x - scrollOffset.x, toPos.y - scrollOffset.y + 70);

        ImVec2 cp1 = ImVec2(fromPos.x + 50, fromPos.y);
        ImVec2 cp2 = ImVec2(toPos.x - 50, toPos.y);

        drawList->AddBezierCubic(fromPos, cp1, cp2, toPos, IM_COL32(200, 200, 100, 255), 3.0f);

        ImVec2 midPoint = ImVec2((fromPos.x + toPos.x) * 0.5f, (fromPos.y + toPos.y) * 0.5f);
        ImGui::SetCursorScreenPos(ImVec2(midPoint.x - 8, midPoint.y - 8));
        ImGui::InvisibleButton(("conn_" + std::to_string(i)).c_str(), ImVec2(16, 16));

        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            connections.erase(connections.begin() + i);
            graph.connections.erase(
                std::remove_if(graph.connections.begin(), graph.connections.end(),
                    [&](const auto& gconn) {
                        return gconn.first == conn.fromID && gconn.second == conn.toID;
                    }),
                graph.connections.end());
            graph.ProcessAll();
            break;
        }
    }

    // Draw input/output pins and handle dragging
    for (const auto& [id, state] : nodeStates) {
        ImVec2 outPin = ImVec2(state.position.x + 200 - scrollOffset.x, state.position.y + 70 - scrollOffset.y);
        ImGui::SetCursorScreenPos(ImVec2(outPin.x - 5, outPin.y - 5));
        ImGui::InvisibleButton(("out_" + std::to_string(id)).c_str(), ImVec2(10, 10));
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            pendingConnection.isDragging = true;
            pendingConnection.fromNodeID = id;
            pendingConnection.startPos = outPin;
        }

        BaseNode* toNode = graph.nodes[id];
        if (BlendNode* blend = dynamic_cast<BlendNode*>(toNode)) {
            // BlendNode has two input pins
            ImVec2 inPinA = ImVec2(state.position.x - scrollOffset.x, state.position.y + 50 - scrollOffset.y);
            ImVec2 inPinB = ImVec2(state.position.x - scrollOffset.x, state.position.y + 90 - scrollOffset.y);

            ImGui::SetCursorScreenPos(ImVec2(inPinA.x - 5, inPinA.y - 5));
            ImGui::InvisibleButton(("inA_" + std::to_string(id)).c_str(), ImVec2(10, 10));
            if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && pendingConnection.isDragging) {
                int fromID = pendingConnection.fromNodeID;
                BaseNode* from = graph.nodes[fromID];
                blend->SetInputA(from->GetOutput());
                connections.push_back({ fromID, id });
                graph.connections.push_back({ fromID, id }); // add to graph manually
                graph.ProcessAll();
                pendingConnection = PendingConnection();
            }

            ImGui::SetCursorScreenPos(ImVec2(inPinB.x - 5, inPinB.y - 5));
            ImGui::InvisibleButton(("inB_" + std::to_string(id)).c_str(), ImVec2(10, 10));
            if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && pendingConnection.isDragging) {
                int fromID = pendingConnection.fromNodeID;
                BaseNode* from = graph.nodes[fromID];
                blend->SetInputB(from->GetOutput());
                connections.push_back({ fromID, id });
                graph.connections.push_back({ fromID, id }); // add to graph manually
                graph.ProcessAll();
                pendingConnection = PendingConnection();
            }

            drawList->AddCircleFilled(inPinA, 5.0f, IM_COL32(0, 255, 255, 255));
            drawList->AddCircleFilled(inPinB, 5.0f, IM_COL32(0, 255, 255, 255));
        }
        else {
            ImVec2 inPin = ImVec2(state.position.x - scrollOffset.x, state.position.y + 70 - scrollOffset.y);
            ImGui::SetCursorScreenPos(ImVec2(inPin.x - 5, inPin.y - 5));
            ImGui::InvisibleButton(("in_" + std::to_string(id)).c_str(), ImVec2(10, 10));
            if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && pendingConnection.isDragging) {
                int fromID = pendingConnection.fromNodeID;
                BaseNode* from = graph.nodes[fromID];
                graph.Connect(fromID, id);
                connections.push_back({ fromID, id });
                graph.ProcessAll();
                pendingConnection = PendingConnection();
            }

            drawList->AddCircleFilled(inPin, 5.0f, IM_COL32(0, 255, 255, 255));
        }

        drawList->AddCircleFilled(outPin, 5.0f, IM_COL32(255, 255, 0, 255));
    }

    // Pending connection wire while dragging
    if (pendingConnection.isDragging) {
        ImVec2 mousePos = ImGui::GetIO().MousePos;
        ImVec2 p1 = pendingConnection.startPos;
        ImVec2 cp1 = ImVec2(p1.x + 50, p1.y);
        ImVec2 cp2 = ImVec2(mousePos.x - 50, mousePos.y);
        drawList->AddBezierCubic(p1, cp1, cp2, mousePos, IM_COL32(255, 255, 0, 255), 2.0f);
    }

    if (pendingConnection.isDragging && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        pendingConnection = PendingConnection();
    }
}



void NodeUIManager::SpawnNode(const std::string& name) {
    static int nextID = 1000;
    BaseNode* newNode = CreateNodeByName(name, nextID++);
    if (!newNode) return;

    nodes.push_back(newNode);
    graph.AddNode(newNode);  // optional if graph.nodes is not populated here
    graph.nodes[newNode->id] = newNode;  // ✅ Critical!
    nodeStates[newNode->id].position = ImVec2(300, 300);

}
