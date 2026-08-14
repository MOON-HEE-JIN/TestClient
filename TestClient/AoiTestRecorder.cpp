#include "AoiTestRecorder.h"

#include "GameDefines.h"

#include <Windows.h>

#include <algorithm>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <system_error>

namespace
{
    constexpr float kPositionTolerance = 5.0f;
    constexpr double kMovementApplyLimitMs = 100.0;
    constexpr double kGridContinuityLimitMs = 100.0;

    enum class TestStatus
    {
        Pass,
        Fail,
        NotTested
    };

    const char* StatusText(TestStatus status)
    {
        switch (status)
        {
        case TestStatus::Pass:
            return "PASS";
        case TestStatus::Fail:
            return "FAIL";
        default:
            return "NOT TESTED";
        }
    }

    std::string PositionText(const st_Vector3F& pos)
    {
        std::ostringstream text;
        text << std::fixed << std::setprecision(2)
            << '(' << pos.X << ", " << pos.Z << ')';
        return text.str();
    }

    std::string TimestampForFile()
    {
        const std::time_t now = std::time(nullptr);
        std::tm localTime{};
        localtime_s(&localTime, &now);

        std::ostringstream text;
        text << std::put_time(&localTime, "%Y%m%d_%H%M%S");
        return text.str();
    }
}

CAoiTestRecorder& CAoiTestRecorder::Get()
{
    static CAoiTestRecorder recorder;
    return recorder;
}

CAoiTestRecorder::~CAoiTestRecorder()
{
    Finalize();
}

void CAoiTestRecorder::Initialize(int autoClientIndex)
{
    std::lock_guard<std::mutex> guard(m_lock);
    if (m_initialized)
        return;

    wchar_t modulePath[MAX_PATH]{};
    const DWORD moduleLength = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    std::filesystem::path outputDirectory;
    if (moduleLength > 0 && moduleLength < MAX_PATH)
        outputDirectory = std::filesystem::path(modulePath).parent_path() / L"AoiTestResults";
    else
        outputDirectory = std::filesystem::current_path() / L"AoiTestResults";

    std::error_code error;
    std::filesystem::create_directories(outputDirectory, error);
    if (error)
        outputDirectory = std::filesystem::current_path();

    m_autoClientIndex = autoClientIndex;
    std::wstring uniqueName = std::filesystem::path(TimestampForFile()).wstring();
    if (m_autoClientIndex >= 0)
        uniqueName += L"_C" + std::to_wstring(m_autoClientIndex);
    uniqueName += L"_PID" + std::to_wstring(GetCurrentProcessId());
    const std::filesystem::path reportPath =
        outputDirectory / (L"AOI_Test_Result_" + uniqueName + L".txt");
    m_reportPath = reportPath.wstring();
    m_stream.open(reportPath, std::ios::binary | std::ios::out | std::ios::trunc);

    m_startTime = Clock::now();
    m_initialized = true;

    if (m_stream.is_open())
    {
        const unsigned char utf8Bom[] = { 0xEF, 0xBB, 0xBF };
        m_stream.write(reinterpret_cast<const char*>(utf8Bom), sizeof(utf8Bom));
        m_stream << "AOI TestClient event log\n";
        if (m_autoClientIndex >= 0)
            m_stream << "Automated client index: " << m_autoClientIndex << "\n";
        m_stream << "Result states: PASS / FAIL / NOT TESTED\n";
        m_stream << "Position tolerance: " << kPositionTolerance << " world units\n";
        m_stream << "Movement packet-to-logic limit: " << kMovementApplyLimitMs << " ms\n";
        m_stream << "Grid continuity sample limit: " << kGridContinuityLimitMs << " ms\n\n";
        m_stream.flush();
    }

    wprintf(L"[AOI TEST] Result file: %ls\n", m_reportPath.c_str());
}

void CAoiTestRecorder::Finalize()
{
    std::lock_guard<std::mutex> guard(m_lock);
    if (!m_initialized || m_finalized)
        return;

    WriteReportLocked(true);
    if (m_stream.is_open())
        m_stream.close();

    m_finalized = true;
    wprintf(L"[AOI TEST] Final result saved: %ls\n", m_reportPath.c_str());
}

void CAoiTestRecorder::SaveSnapshot()
{
    std::lock_guard<std::mutex> guard(m_lock);
    if (!m_initialized || m_finalized)
        return;

    WriteReportLocked(false);
    wprintf(L"[AOI TEST] Snapshot saved: %ls\n", m_reportPath.c_str());
}

std::wstring CAoiTestRecorder::GetReportPath() const
{
    std::lock_guard<std::mutex> guard(m_lock);
    return m_reportPath;
}

void CAoiTestRecorder::SetLocalPlayerID(int id)
{
    std::lock_guard<std::mutex> guard(m_lock);
    m_localPlayerID = id;
    LogEventLocked("LOCAL_ID id=" + std::to_string(id));
}

void CAoiTestRecorder::SetLocalPosition(const st_Vector3F& pos)
{
    std::lock_guard<std::mutex> guard(m_lock);
    m_localPos = pos;
    m_localTileX = TileX(pos);
    m_localTileZ = TileZ(pos);
    m_localGridID = GridID(pos);
    m_localLastSample = Clock::now();
    m_hasLocalPosition = true;
    m_localTeleportPending = false;
    m_localWaitingForPostGridSample = false;
    LogEventLocked("LOCAL_POSITION_RESET pos=" + PositionText(pos));
}

void CAoiTestRecorder::BeginLocalTeleport(const st_Vector3F& expectedPos)
{
    std::lock_guard<std::mutex> guard(m_lock);
    m_localTeleportPending = true;
    m_expectedTeleportPos = expectedPos;
    LogEventLocked("LOCAL_TELEPORT_PENDING expected_pos=" + PositionText(expectedPos));
}

void CAoiTestRecorder::RecordLocalPosition(const st_Vector3F& pos)
{
    std::lock_guard<std::mutex> guard(m_lock);
    const Clock::time_point now = Clock::now();
    const int newTileX = TileX(pos);
    const int newTileZ = TileZ(pos);
    const int newGridID = GridID(pos);

    if (!m_hasLocalPosition)
    {
        m_localPos = pos;
        m_localTileX = newTileX;
        m_localTileZ = newTileZ;
        m_localGridID = newGridID;
        m_localLastSample = now;
        m_hasLocalPosition = true;
        return;
    }

    if (newTileX != m_localTileX || newTileZ != m_localTileZ)
    {
        ++m_localTileTransitions;
        LogEventLocked("LOCAL_TILE from=(" + std::to_string(m_localTileX) + ',' +
            std::to_string(m_localTileZ) + ") to=(" + std::to_string(newTileX) + ',' +
            std::to_string(newTileZ) + ") pos=" + PositionText(pos));
    }

    if (m_localMoving && newGridID != m_localGridID)
    {
        ++m_localGridTransitions;
        m_localCrossedGridDuringMove = true;
        m_localWaitingForPostGridSample = true;
        m_localGridCrossTime = now;

        const double gapMs = std::chrono::duration<double, std::milli>(now - m_localLastSample).count();
        m_maxGridSampleGapMs = (std::max)(m_maxGridSampleGapMs, gapMs);
        if (gapMs > kGridContinuityLimitMs)
            ++m_gridGapFailures;

        LogEventLocked("LOCAL_GRID from=" + std::to_string(m_localGridID) +
            " to=" + std::to_string(newGridID) + " sample_gap_ms=" + std::to_string(gapMs));
    }
    else if (m_localMoving && m_localWaitingForPostGridSample && Distance(m_localPos, pos) > 0.0f)
    {
        const double gapMs = std::chrono::duration<double, std::milli>(now - m_localGridCrossTime).count();
        m_maxGridSampleGapMs = (std::max)(m_maxGridSampleGapMs, gapMs);
        if (gapMs > kGridContinuityLimitMs)
            ++m_gridGapFailures;
        ++m_gridContinuitySamples;
        m_localWaitingForPostGridSample = false;
        LogEventLocked("LOCAL_GRID_CONTINUED gap_ms=" + std::to_string(gapMs));
    }

    m_localPos = pos;
    m_localTileX = newTileX;
    m_localTileZ = newTileZ;
    m_localGridID = newGridID;
    m_localLastSample = now;
}

void CAoiTestRecorder::RecordLocalMoveStartRequest(const st_Vector3F& pos, const st_Vector3F& dir)
{
    std::lock_guard<std::mutex> guard(m_lock);
    ++m_localMoveStartRequests;
    if (!m_localMoving)
        m_localCrossedGridDuringMove = false;
    m_localMoving = true;

    if (!IsValidDirection(dir))
        ++m_invalidDirections;

    LogEventLocked("LOCAL_MOVE_START_REQ pos=" + PositionText(pos) +
        " dir=(" + std::to_string(dir.X) + ", " + std::to_string(dir.Z) + ')');
}

void CAoiTestRecorder::RecordLocalMoveStopRequest(const st_Vector3F& pos)
{
    std::lock_guard<std::mutex> guard(m_lock);
    ++m_localMoveStopRequests;
    LogEventLocked("LOCAL_MOVE_STOP_REQ pos=" + PositionText(pos));
}

void CAoiTestRecorder::RecordLocalMoveStartResponse(int ret, const st_Vector3F& pos)
{
    std::lock_guard<std::mutex> guard(m_lock);
    ++m_localMoveStartResponses;
    if (ret != 0)
        ++m_localMoveStartFailures;

    ++m_positionChecks;
    const float correction = Distance(m_localPos, pos);
    m_maxPositionCorrection = (std::max)(m_maxPositionCorrection, static_cast<double>(correction));
    if (correction > kPositionTolerance)
    {
        ++m_positionFailures;
        ++m_localPositionFailures;
    }

    LogEventLocked("LOCAL_MOVE_START_RES ret=" + std::to_string(ret) +
        " server_pos=" + PositionText(pos) + " correction=" + std::to_string(correction));
}

void CAoiTestRecorder::RecordAoiIn(const st_PlayerInfo& info, bool batchPacket)
{
    std::lock_guard<std::mutex> guard(m_lock);
    if (info.ID == m_localPlayerID)
    {
        ++m_selfAoiPackets;
        LogEventLocked("ERROR AOI_IN contains local id=" + std::to_string(info.ID));
        return;
    }

    PlayerState& player = m_players[info.ID];
    if (player.visible)
        ++m_duplicateAoiIn;

    if (!IsValidPosition(info.pos) || !std::isfinite(info.speed) || info.speed <= 0.0f)
        ++m_invalidAoiPositions;

    if (m_hasLocalPosition)
    {
        const int referenceTileX = m_localTeleportPending ? TileX(m_expectedTeleportPos) : m_localTileX;
        const int referenceTileZ = m_localTeleportPending ? TileZ(m_expectedTeleportPos) : m_localTileZ;
        const int tileDistanceX = std::abs(TileX(info.pos) - referenceTileX);
        const int tileDistanceZ = std::abs(TileZ(info.pos) - referenceTileZ);
        if (tileDistanceX > AOI_TILE_RADIUS + 1 || tileDistanceZ > AOI_TILE_RADIUS + 1)
            ++m_outOfViewAoiIn;
    }

    ++m_aoiInCount;
    if (batchPacket)
        ++m_batchAoiInCount;

    player.pos = info.pos;
    player.initialPos = info.pos;
    player.tileX = TileX(info.pos);
    player.tileZ = TileZ(info.pos);
    player.gridID = GridID(info.pos);
    player.lastSample = Clock::now();
    player.visible = true;
    player.moving = false;
    player.hasSample = true;
    player.freshlyEntered = true;
    player.initialLogicApplied = false;
    player.initialRenderApplied = false;
    player.initialSyncCounted = false;
    player.moveStartApplyPending = false;
    player.moveStopApplyPending = false;
    player.crossedGridDuringMove = false;
    player.waitingForPostGridSample = false;

    LogEventLocked("AOI_IN id=" + std::to_string(info.ID) + " pos=" + PositionText(info.pos) +
        " speed=" + std::to_string(info.speed) + (batchPacket ? " packet=batch" : " packet=single"));
}

void CAoiTestRecorder::RecordAoiOut(int id, bool batchPacket)
{
    std::lock_guard<std::mutex> guard(m_lock);
    if (id == m_localPlayerID)
    {
        ++m_selfAoiPackets;
        LogEventLocked("ERROR AOI_OUT contains local id=" + std::to_string(id));
        return;
    }

    const auto it = m_players.find(id);
    if (it == m_players.end() || !it->second.visible)
        ++m_unknownAoiOut;
    else
        it->second.visible = false;

    ++m_aoiOutCount;
    if (batchPacket)
        ++m_batchAoiOutCount;
    LogEventLocked("AOI_OUT id=" + std::to_string(id) + (batchPacket ? " packet=batch" : " packet=single"));
}

void CAoiTestRecorder::RecordOtherCreated(int id, const st_Vector3F& pos)
{
    std::lock_guard<std::mutex> guard(m_lock);
    auto it = m_players.find(id);
    if (it == m_players.end() || !it->second.visible)
    {
        ++m_logicErrors;
        LogEventLocked("ERROR OTHER_CREATED without AOI_IN id=" + std::to_string(id));
        return;
    }

    PlayerState& player = it->second;
    if (player.initialLogicApplied)
        return;

    ++m_initialLogicChecks;
    const float correction = Distance(player.initialPos, pos);
    if (correction > 0.01f)
        ++m_initialLogicFailures;
    player.initialLogicApplied = true;

    if (player.initialRenderApplied && !player.initialSyncCounted)
    {
        ++m_completedInitialSync;
        player.initialSyncCounted = true;
    }

    LogEventLocked("OTHER_CREATED id=" + std::to_string(id) +
        " initial_correction=" + std::to_string(correction));
}

void CAoiTestRecorder::RecordOtherRendered(int id, const st_Vector3F& pos)
{
    std::lock_guard<std::mutex> guard(m_lock);
    auto it = m_players.find(id);
    if (it == m_players.end() || !it->second.visible)
        return;

    PlayerState& player = it->second;
    if (player.initialRenderApplied)
        return;

    ++m_initialRenderChecks;
    const float correction = Distance(player.initialPos, pos);
    if (correction > 0.01f)
        ++m_initialRenderFailures;
    player.initialRenderApplied = true;

    if (player.initialLogicApplied && !player.initialSyncCounted)
    {
        ++m_completedInitialSync;
        player.initialSyncCounted = true;
    }

    LogEventLocked("OTHER_RENDERED id=" + std::to_string(id) +
        " initial_correction=" + std::to_string(correction));
}

void CAoiTestRecorder::RecordOtherMoveStart(
    int id, const st_Vector3F& pos, const st_Vector3F& dir)
{
    std::lock_guard<std::mutex> guard(m_lock);
    auto it = m_players.find(id);
    if (it == m_players.end() || !it->second.visible)
    {
        ++m_unknownMoveStarts;
        LogEventLocked("ERROR REMOTE_MOVE_START unknown id=" + std::to_string(id));
        return;
    }

    if (!IsValidDirection(dir))
        ++m_invalidDirections;

    PlayerState& player = it->second;
    ++m_positionChecks;
    const float correction = Distance(player.pos, pos);
    m_maxPositionCorrection = (std::max)(m_maxPositionCorrection, static_cast<double>(correction));
    if (correction > kPositionTolerance)
    {
        ++m_positionFailures;
        ++m_remotePositionFailures;
    }

    if (player.freshlyEntered)
    {
        ++m_initialPositionChecks;
        if (correction > kPositionTolerance)
            ++m_initialPositionFailures;
    }

    ++m_remoteMoveStarts;
    if (!player.moving)
        player.crossedGridDuringMove = false;
    player.moving = true;
    player.pos = pos;
    player.tileX = TileX(pos);
    player.tileZ = TileZ(pos);
    player.gridID = GridID(pos);
    player.lastSample = Clock::now();
    player.moveStartPacketTime = Clock::now();
    player.moveStartApplyPending = true;
    player.hasSample = true;
    player.freshlyEntered = false;

    LogEventLocked("REMOTE_MOVE_START id=" + std::to_string(id) + " pos=" + PositionText(pos) +
        " correction=" + std::to_string(correction));
}

void CAoiTestRecorder::RecordOtherMoveStartApplied(int id)
{
    std::lock_guard<std::mutex> guard(m_lock);
    auto it = m_players.find(id);
    if (it == m_players.end() || !it->second.moveStartApplyPending)
    {
        ++m_logicErrors;
        ++m_moveApplyFailures;
        LogEventLocked("ERROR REMOTE_MOVE_START apply without packet id=" + std::to_string(id));
        return;
    }

    PlayerState& player = it->second;
    const double delayMs = std::chrono::duration<double, std::milli>(
        Clock::now() - player.moveStartPacketTime).count();
    ++m_moveStartApplyChecks;
    m_maxMoveApplyDelayMs = (std::max)(m_maxMoveApplyDelayMs, delayMs);
    if (delayMs > kMovementApplyLimitMs)
        ++m_moveApplyFailures;
    player.moveStartApplyPending = false;
    LogEventLocked("REMOTE_MOVE_START_APPLIED id=" + std::to_string(id) +
        " delay_ms=" + std::to_string(delayMs));
}

void CAoiTestRecorder::RecordMoveStopResponse(int id, int ret, const st_Vector3F& pos)
{
    std::lock_guard<std::mutex> guard(m_lock);
    if (id == m_localPlayerID)
    {
        ++m_localMoveStopResponses;
        ++m_positionChecks;
        const float correction = Distance(m_localPos, pos);
        m_maxPositionCorrection = (std::max)(m_maxPositionCorrection, static_cast<double>(correction));
        if (ret != 0 || correction > kPositionTolerance)
        {
            ++m_positionFailures;
            ++m_localPositionFailures;
            if (m_localCrossedGridDuringMove)
                ++m_gridPositionFailures;
        }
        else if (m_localCrossedGridDuringMove)
            ++m_completedGridMoveSessions;

        m_localMoving = false;
        m_localCrossedGridDuringMove = false;
        m_localWaitingForPostGridSample = false;
        LogEventLocked("LOCAL_MOVE_STOP_RES ret=" + std::to_string(ret) + " pos=" +
            PositionText(pos) + " correction=" + std::to_string(correction));
        return;
    }

    auto it = m_players.find(id);
    if (it == m_players.end() || !it->second.visible)
    {
        ++m_unknownMoveStops;
        LogEventLocked("ERROR REMOTE_MOVE_STOP unknown id=" + std::to_string(id));
        return;
    }

    PlayerState& player = it->second;
    player.moveStopPacketTime = Clock::now();
    player.moveStopApplyPending = true;
    ++m_remoteMoveStops;
    if (player.moving)
        ++m_remoteMovePairs;
    else
        ++m_unknownMoveStops;

    ++m_positionChecks;
    const float correction = Distance(player.pos, pos);
    m_maxPositionCorrection = (std::max)(m_maxPositionCorrection, static_cast<double>(correction));
    if (ret != 0 || correction > kPositionTolerance)
    {
        ++m_positionFailures;
        ++m_remotePositionFailures;
        if (player.crossedGridDuringMove)
            ++m_gridPositionFailures;
    }
    else if (player.crossedGridDuringMove)
        ++m_completedGridMoveSessions;

    player.pos = pos;
    player.tileX = TileX(pos);
    player.tileZ = TileZ(pos);
    player.gridID = GridID(pos);
    player.moving = false;
    player.freshlyEntered = false;
    player.crossedGridDuringMove = false;
    player.waitingForPostGridSample = false;

    LogEventLocked("REMOTE_MOVE_STOP id=" + std::to_string(id) + " ret=" + std::to_string(ret) +
        " pos=" + PositionText(pos) + " correction=" + std::to_string(correction));
}

void CAoiTestRecorder::RecordOtherMoveStopApplied(int id)
{
    std::lock_guard<std::mutex> guard(m_lock);
    auto it = m_players.find(id);
    if (it == m_players.end() || !it->second.moveStopApplyPending)
    {
        ++m_logicErrors;
        ++m_moveApplyFailures;
        LogEventLocked("ERROR REMOTE_MOVE_STOP apply without packet id=" + std::to_string(id));
        return;
    }

    PlayerState& player = it->second;
    const double delayMs = std::chrono::duration<double, std::milli>(
        Clock::now() - player.moveStopPacketTime).count();
    ++m_moveStopApplyChecks;
    m_maxMoveApplyDelayMs = (std::max)(m_maxMoveApplyDelayMs, delayMs);
    if (delayMs > kMovementApplyLimitMs)
        ++m_moveApplyFailures;
    player.moveStopApplyPending = false;
    LogEventLocked("REMOTE_MOVE_STOP_APPLIED id=" + std::to_string(id) +
        " delay_ms=" + std::to_string(delayMs));
}

void CAoiTestRecorder::RecordOtherPosition(int id, const st_Vector3F& pos)
{
    std::lock_guard<std::mutex> guard(m_lock);
    auto it = m_players.find(id);
    if (it == m_players.end() || !it->second.visible)
        return;

    PlayerState& player = it->second;
    const Clock::time_point now = Clock::now();
    const int newTileX = TileX(pos);
    const int newTileZ = TileZ(pos);
    const int newGridID = GridID(pos);

    if (player.hasSample && (newTileX != player.tileX || newTileZ != player.tileZ))
    {
        ++m_remoteTileTransitions;
        LogEventLocked("REMOTE_TILE id=" + std::to_string(id) + " from=(" +
            std::to_string(player.tileX) + ',' + std::to_string(player.tileZ) + ") to=(" +
            std::to_string(newTileX) + ',' + std::to_string(newTileZ) + ')');
    }

    if (player.moving && player.hasSample && newGridID != player.gridID)
    {
        ++m_remoteGridTransitions;
        player.crossedGridDuringMove = true;
        player.waitingForPostGridSample = true;
        player.gridCrossTime = now;

        const double gapMs = std::chrono::duration<double, std::milli>(now - player.lastSample).count();
        m_maxGridSampleGapMs = (std::max)(m_maxGridSampleGapMs, gapMs);
        if (gapMs > kGridContinuityLimitMs)
            ++m_gridGapFailures;
        LogEventLocked("REMOTE_GRID id=" + std::to_string(id) + " from=" +
            std::to_string(player.gridID) + " to=" + std::to_string(newGridID) +
            " sample_gap_ms=" + std::to_string(gapMs));
    }
    else if (player.moving && player.waitingForPostGridSample && Distance(player.pos, pos) > 0.0f)
    {
        const double gapMs = std::chrono::duration<double, std::milli>(now - player.gridCrossTime).count();
        m_maxGridSampleGapMs = (std::max)(m_maxGridSampleGapMs, gapMs);
        if (gapMs > kGridContinuityLimitMs)
            ++m_gridGapFailures;
        ++m_gridContinuitySamples;
        player.waitingForPostGridSample = false;
        LogEventLocked("REMOTE_GRID_CONTINUED id=" + std::to_string(id) +
            " gap_ms=" + std::to_string(gapMs));
    }

    player.pos = pos;
    player.tileX = newTileX;
    player.tileZ = newTileZ;
    player.gridID = newGridID;
    player.lastSample = now;
    player.hasSample = true;
}

void CAoiTestRecorder::RecordLogicError(const std::string& message)
{
    std::lock_guard<std::mutex> guard(m_lock);
    ++m_logicErrors;
    LogEventLocked("ERROR LOGIC " + message);
}

bool CAoiTestRecorder::IsValidPosition(const st_Vector3F& pos)
{
    return std::isfinite(pos.X) && std::isfinite(pos.Y) && std::isfinite(pos.Z) &&
        pos.X >= 0.0f && pos.X < static_cast<float>(MAP_WIDTH) &&
        pos.Z >= 0.0f && pos.Z < static_cast<float>(MAP_HEIGHT);
}

bool CAoiTestRecorder::IsValidDirection(const st_Vector3F& dir)
{
    if (!std::isfinite(dir.X) || !std::isfinite(dir.Y) || !std::isfinite(dir.Z))
        return false;

    const float length = std::sqrt(dir.X * dir.X + dir.Y * dir.Y + dir.Z * dir.Z);
    return std::abs(length - 1.0f) <= 0.05f;
}

float CAoiTestRecorder::Distance(const st_Vector3F& lhs, const st_Vector3F& rhs)
{
    const float x = lhs.X - rhs.X;
    const float y = lhs.Y - rhs.Y;
    const float z = lhs.Z - rhs.Z;
    return std::sqrt(x * x + y * y + z * z);
}

int CAoiTestRecorder::TileX(const st_Vector3F& pos)
{
    return (std::clamp)(static_cast<int>(pos.X) / TILE_SIZE, 0, TILE_COUNT_X - 1);
}

int CAoiTestRecorder::TileZ(const st_Vector3F& pos)
{
    return (std::clamp)(static_cast<int>(pos.Z) / TILE_SIZE, 0, TILE_COUNT_Y - 1);
}

int CAoiTestRecorder::GridID(const st_Vector3F& pos)
{
    return (TileZ(pos) * GRID_COUNT) / TILE_COUNT_Y;
}

void CAoiTestRecorder::LogEventLocked(const std::string& event)
{
    if (!m_stream.is_open())
        return;

    const double elapsedMs =
        std::chrono::duration<double, std::milli>(Clock::now() - m_startTime).count();
    m_stream << '[' << std::fixed << std::setprecision(3) << elapsedMs << " ms] " << event << '\n';
    m_stream.flush();
}

void CAoiTestRecorder::WriteReportLocked(bool finalReport)
{
    if (!m_stream.is_open())
        return;

    const int movementErrors = m_unknownMoveStarts + m_unknownMoveStops +
        m_invalidDirections + m_remotePositionFailures + m_localMoveStartFailures +
        m_localPositionFailures + m_moveApplyFailures;
    const int aoiErrors = m_duplicateAoiIn + m_unknownAoiOut + m_selfAoiPackets +
        m_invalidAoiPositions + m_outOfViewAoiIn + m_logicErrors;

    const bool movementObserved = m_remoteMoveStarts > 0 && m_remoteMovePairs > 0;
    const bool tileAoiObserved = (m_localTileTransitions + m_remoteTileTransitions) > 0 &&
        m_aoiInCount > 0 && m_aoiOutCount > 0;
    const bool initialPositionObserved = m_completedInitialSync > 0;
    const bool gridObserved = (m_localGridTransitions + m_remoteGridTransitions) > 0 &&
        m_gridContinuitySamples > 0 && m_completedGridMoveSessions > 0;

    const bool movementPacketObserved =
        (m_remoteMoveStarts + m_remoteMoveStops + m_unknownMoveStarts + m_unknownMoveStops +
            m_localMoveStartResponses + m_localMoveStopResponses) > 0;
    const TestStatus movementStatus = movementErrors > 0 && movementPacketObserved ? TestStatus::Fail :
        (movementObserved ? TestStatus::Pass : TestStatus::NotTested);
    const TestStatus tileAoiStatus = aoiErrors > 0 ? TestStatus::Fail :
        (tileAoiObserved ? TestStatus::Pass : TestStatus::NotTested);
    const TestStatus initialPositionStatus =
        (m_invalidAoiPositions + m_duplicateAoiIn + m_initialPositionFailures +
            m_initialLogicFailures + m_initialRenderFailures + m_logicErrors) > 0 ?
        TestStatus::Fail : (initialPositionObserved ? TestStatus::Pass : TestStatus::NotTested);
    const bool gridCrossingObserved = (m_localGridTransitions + m_remoteGridTransitions) > 0;
    const TestStatus gridStatus = !gridCrossingObserved ? TestStatus::NotTested :
        ((m_gridGapFailures + m_gridPositionFailures) > 0 ? TestStatus::Fail :
            (gridObserved ? TestStatus::Pass : TestStatus::NotTested));

    int passCount = 0;
    int failCount = 0;
    int notTestedCount = 0;
    for (TestStatus status : { movementStatus, tileAoiStatus, initialPositionStatus, gridStatus })
    {
        if (status == TestStatus::Pass)
            ++passCount;
        else if (status == TestStatus::Fail)
            ++failCount;
        else
            ++notTestedCount;
    }

    m_stream << "\n============================================================\n";
    m_stream << (finalReport ? "FINAL AOI TEST RESULT\n" : "AOI TEST SNAPSHOT\n");
    m_stream << "Overall: PASS=" << passCount << " FAIL=" << failCount
        << " NOT_TESTED=" << notTestedCount << "\n";
    if (m_autoClientIndex >= 0)
        m_stream << "Automated client index: " << m_autoClientIndex << "\n";
    m_stream << "============================================================\n\n";

    m_stream << "[1] Other-player move start/stop synchronization: "
        << StatusText(movementStatus) << "\n";
    m_stream << "  1.1 MoveStart reception: starts=" << m_remoteMoveStarts
        << ", unknown_id=" << m_unknownMoveStarts << "\n";
    m_stream << "  1.2 MoveStop pairing: stops=" << m_remoteMoveStops
        << ", valid_pairs=" << m_remoteMovePairs << ", invalid_sequence=" << m_unknownMoveStops << "\n";
    m_stream << "  1.3 Direction validity: invalid=" << m_invalidDirections << "\n";
    m_stream << "  1.4 Position correction: checks=" << m_positionChecks
        << ", over_tolerance=" << m_positionFailures << ", max="
        << std::fixed << std::setprecision(3) << m_maxPositionCorrection << "\n";
    m_stream << "  1.5 Packet-to-logic apply: start_checks=" << m_moveStartApplyChecks
        << ", stop_checks=" << m_moveStopApplyChecks << ", over_" << kMovementApplyLimitMs
        << "ms=" << m_moveApplyFailures << ", max_ms=" << m_maxMoveApplyDelayMs << "\n";
    m_stream << "  1.6 Local request/response: start_req=" << m_localMoveStartRequests
        << ", start_res=" << m_localMoveStartResponses << ", start_fail=" << m_localMoveStartFailures
        << ", stop_req=" << m_localMoveStopRequests << ", stop_res=" << m_localMoveStopResponses << "\n\n";

    m_stream << "[2] AOI update caused by Tile movement: " << StatusText(tileAoiStatus) << "\n";
    m_stream << "  2.1 Tile transitions: local=" << m_localTileTransitions
        << ", remote=" << m_remoteTileTransitions << "\n";
    m_stream << "  2.2 AOI enter/leave: in=" << m_aoiInCount << " (batch=" << m_batchAoiInCount
        << "), out=" << m_aoiOutCount << " (batch=" << m_batchAoiOutCount << ")\n";
    m_stream << "  2.3 Lifecycle validity: duplicate_in=" << m_duplicateAoiIn
        << ", unknown_out=" << m_unknownAoiOut << ", self_packets=" << m_selfAoiPackets << "\n";
    m_stream << "  2.4 AOI range check: outside_allowed_range=" << m_outOfViewAoiIn << "\n\n";

    m_stream << "[3] Initial position synchronization for AOI-enter player: "
        << StatusText(initialPositionStatus) << "\n";
    m_stream << "  3.1 AOI-in positions received=" << m_aoiInCount
        << ", invalid_position_or_speed=" << m_invalidAoiPositions << "\n";
    m_stream << "  3.2 Creation/order errors: duplicate_in=" << m_duplicateAoiIn
        << ", logic_errors=" << m_logicErrors << "\n";
    m_stream << "  3.3 Initial application: logic_checks=" << m_initialLogicChecks
        << ", logic_failures=" << m_initialLogicFailures << ", render_checks=" << m_initialRenderChecks
        << ", render_failures=" << m_initialRenderFailures
        << ", completed=" << m_completedInitialSync << "\n";
    m_stream << "  3.4 First movement correction uses tolerance=" << kPositionTolerance
        << ", checks=" << m_initialPositionChecks << ", failures=" << m_initialPositionFailures << "\n\n";

    m_stream << "[4] Continuous movement across server Grid boundaries: "
        << StatusText(gridStatus) << "\n";
    m_stream << "  4.1 Grid transitions: local=" << m_localGridTransitions
        << ", remote=" << m_remoteGridTransitions << "\n";
    m_stream << "  4.2 Post-boundary continuity samples=" << m_gridContinuitySamples
        << ", gap_failures=" << m_gridGapFailures << ", max_gap_ms="
        << std::fixed << std::setprecision(3) << m_maxGridSampleGapMs << "\n";
    m_stream << "  4.3 Authoritative MoveStop after crossing=" << m_completedGridMoveSessions << "\n";
    m_stream << "  4.4 Authoritative position failures after crossing=" << m_gridPositionFailures << "\n";
    m_stream << "  Grid boundaries are Z=256, 512, 768 (Tile rows 4, 8, 12).\n\n";

    m_stream << "Execution guidance\n";
    m_stream << "  - Use another client/player in the same zone so remote movement packets are observable.\n";
    m_stream << "  - Cross Tile boundaries at multiples of 64 in X or Z and verify AOI enter/leave.\n";
    m_stream << "  - Cross Grid boundaries at Z=256, 512, or 768 while continuously moving.\n";
    m_stream << "  - Stop after each crossing so the authoritative server position can be compared.\n";
    m_stream << "  - NOT TESTED means the required packet/boundary sequence was not observed; it is not a pass.\n";
    m_stream << "============================================================\n";
    m_stream.flush();
}
