#include "TrackingRenderer.h"
#include "Colors.h"
#include "OpencvUtils.h"
#include "BlackSegmentationRenderer.h"
#include "WinStyleSegmentationRenderer.h"
#include <string>
#include <sstream>
#include <iomanip>

TrackingRenderer::TrackingRenderer(Viewer& viewer) : m_viewer(viewer)
{
    viewer.SetMouseEventHandler([this] (int event, int x, int y, int flags)
    {
        HandleMouseEvent(event, x, y, flags);
    });
}

void TrackingRenderer::Reset()
{
    mLastCenterOfMassTextBottom = 0;
    //m_personDataStorage.clear();
}

void TrackingRenderer::DrawPerson(cv::Mat image, int id, cv::Rect boundingBox, cv::Point com, cv::Point3f comWorld)
{
    cv::Point pt1(boundingBox.x, boundingBox.y);
    cv::Point pt2(boundingBox.x + boundingBox.width, boundingBox.y + boundingBox.height);

    cv::rectangle(image, boundingBox, YELLOW, 2);

    // person ID at top left corner of person rectangle
    int personId = id;
    std::string idText = "ID: " + std::to_string(personId);
    Rectangle personIdTextRect = setLabel(image, idText, pt1 + cv::Point(5, 5), 1, 0.4);

    // recognition ID under person ID
    auto personData = m_personDataStorage.get(id);
    int rid = (personData != nullptr && personData->rid > 0) ? personData->rid : -1;
    {
        std::string recognitionIdText = "RID: " + ((rid > 0) ? std::to_string(rid) : std::string("?"));
        setLabel(image, recognitionIdText, pt1 + cv::Point(5, 5 + personIdTextRect.height), 1, 0.4);
    }

    // center of mass point
    cv::Point centerMass = com;
    cv::circle(image, centerMass, 2, RED, -1, 8, 0);

    // center of mass world coordinates at top left corner of image
    std::stringstream centerMassText;
    centerMassText <<
        personId << ": " <<
        std::fixed << std::setprecision(3) <<
        "(" << comWorld.x << "," << comWorld.y << "," << comWorld.z << ")";
    Rectangle centerMassTextRect = setLabel(image, centerMassText.str(), cv::Point(0, mLastCenterOfMassTextBottom), 1);
    mLastCenterOfMassTextBottom = centerMassTextRect.bottomRight.y;

    // add person data to storage
    PersonData data;
    data.Id = personId;
    data.rid = rid;
    data.rectangle = boundingBox;
    m_personDataStorage.add(data);
}

void TrackingRenderer::DrawSkeleton(cv::Mat image, std::vector<cv::Point>& points)
{
    for (auto point : points)
    {
        cv::circle(image, point, 3, GREEN, -1, 8, 0);
    }
}

void TrackingRenderer::DrawPointing(cv::Mat image, cv::Point origin, cv::Point2f direction)
{
    cv::circle(image, origin, 3, BLUE, -1, 8, 0);
    cv::Point vector(origin.x + 400 * direction.x, origin.y + 400 * direction.y);
    cv::arrowedLine(image, origin, vector, GREEN, 2, 8, 0, 0.15);
}

void TrackingRenderer::DrawSegmentation(cv::Mat image, std::vector<cv::Mat>& segmentedImages)
{
    if (m_segmentationRenderer) m_segmentationRenderer->RenderSegmentation(image, segmentedImages);
}

void TrackingRenderer::SetSegmentationType(SegmentationRenderer::Type type)
{
    if (type == SegmentationRenderer::Type::NONE)
    {
        m_segmentationRenderer = nullptr;
    }
    if (type == SegmentationRenderer::Type::BLACK)
    {
        m_segmentationRenderer = std::unique_ptr<SegmentationRenderer>(new BlackSegmentationRenderer());
    }
    if (type == SegmentationRenderer::Type::WIN_STYLE)
    {
        m_segmentationRenderer = std::unique_ptr<SegmentationRenderer>(new WinStyleSegmentationRenderer());
    }
}

void TrackingRenderer::HandleMouseEvent(int event, int x, int y, int flags)
{
    if (!m_personSelectedHandler) return;

    PersonData* data = m_personDataStorage.matchPersonToPoint(cv::Point(x,y));
    if (data == nullptr) return;

    if  (event == cv::EVENT_LBUTTONDOWN)
    {
        m_personSelectedHandler(*data, SelectType::RECOGNITION);
    }
    else if  (event == cv::EVENT_MBUTTONDOWN)
    {
        m_personSelectedHandler(*data, SelectType::TRACKING);
    }
}
