#pragma once

#include "Stub/StructDef.h"

#include <chrono>
#include <fstream>
#include <map>
#include <mutex>
#include <string>

class CAoiTestRecorder
{
public:
    static CAoiTestRecorder& Get();

    void Initialize(int autoClientIndex = -1);
    void Finalize();
    void SaveSnapshot();

    std::wstring GetReportPath() const;

    void SetLocalPlayerID(int id);
    void SetLocalPosition(const st_Vector3F& pos);
    void BeginLocalTeleport(const st_Vector3F& expectedPos);
    void RecordLocalPosition(const st_Vector3F& pos);
    void RecordLocalMoveStartRequest(const st_Vector3F& pos, const st_Vector3F& dir);
    void RecordLocalMoveStopRequest(const st_Vector3F& pos);
    void RecordLocalMoveStartResponse(int ret, const st_Vector3F& pos);

    void RecordAoiIn(const st_PlayerInfo& info, bool batchPacket);
    void RecordAoiOut(int id, bool batchPacket);
    void RecordOtherCreated(int id, const st_Vector3F& pos);
    void RecordOtherRendered(int id, const st_Vector3F& pos);
    void RecordOtherMoveStart(int id, const st_Vector3F& pos, const st_Vector3F& dir);
    void RecordOtherMoveStartApplied(int id);
    void RecordMoveStopResponse(int id, int ret, const st_Vector3F& pos);
    void RecordOtherMoveStopApplied(int id);
    void RecordOtherPosition(int id, const st_Vector3F& pos);
    void RecordLogicError(const std::string& message);

private:
    using Clock = std::chrono::steady_clock;

    struct PlayerState
    {
        st_Vector3F pos{};
        st_Vector3F initialPos{};
        Clock::time_point lastSample{};
        Clock::time_point gridCrossTime{};
        Clock::time_point moveStartPacketTime{};
        Clock::time_point moveStopPacketTime{};
        int tileX = -1;
        int tileZ = -1;
        int gridID = -1;
        bool visible = false;
        bool moving = false;
        bool hasSample = false;
        bool freshlyEntered = false;
        bool initialLogicApplied = false;
        bool initialRenderApplied = false;
        bool initialSyncCounted = false;
        bool moveStartApplyPending = false;
        bool moveStopApplyPending = false;
        bool crossedGridDuringMove = false;
        bool waitingForPostGridSample = false;
    };

    CAoiTestRecorder() = default;
    ~CAoiTestRecorder();
    CAoiTestRecorder(const CAoiTestRecorder&) = delete;
    CAoiTestRecorder& operator=(const CAoiTestRecorder&) = delete;

    static bool IsValidPosition(const st_Vector3F& pos);
    static bool IsValidDirection(const st_Vector3F& dir);
    static float Distance(const st_Vector3F& lhs, const st_Vector3F& rhs);
    static int TileX(const st_Vector3F& pos);
    static int TileZ(const st_Vector3F& pos);
    static int GridID(const st_Vector3F& pos);

    void LogEventLocked(const std::string& event);
    void WriteReportLocked(bool finalReport);

    mutable std::mutex m_lock;
    std::ofstream m_stream;
    std::wstring m_reportPath;
    Clock::time_point m_startTime{};
    bool m_initialized = false;
    bool m_finalized = false;
    int m_autoClientIndex = -1;

    int m_localPlayerID = -1;
    st_Vector3F m_localPos{};
    Clock::time_point m_localLastSample{};
    Clock::time_point m_localGridCrossTime{};
    int m_localTileX = -1;
    int m_localTileZ = -1;
    int m_localGridID = -1;
    bool m_hasLocalPosition = false;
    bool m_localTeleportPending = false;
    st_Vector3F m_expectedTeleportPos{};
    bool m_localMoving = false;
    bool m_localCrossedGridDuringMove = false;
    bool m_localWaitingForPostGridSample = false;

    std::map<int, PlayerState> m_players;

    int m_localMoveStartRequests = 0;
    int m_localMoveStopRequests = 0;
    int m_localMoveStartResponses = 0;
    int m_localMoveStartFailures = 0;
    int m_localMoveStopResponses = 0;

    int m_remoteMoveStarts = 0;
    int m_remoteMoveStops = 0;
    int m_remoteMovePairs = 0;
    int m_unknownMoveStarts = 0;
    int m_unknownMoveStops = 0;
    int m_invalidDirections = 0;
    int m_moveStartApplyChecks = 0;
    int m_moveStopApplyChecks = 0;
    int m_moveApplyFailures = 0;
    double m_maxMoveApplyDelayMs = 0.0;

    int m_aoiInCount = 0;
    int m_aoiOutCount = 0;
    int m_batchAoiInCount = 0;
    int m_batchAoiOutCount = 0;
    int m_duplicateAoiIn = 0;
    int m_unknownAoiOut = 0;
    int m_selfAoiPackets = 0;
    int m_invalidAoiPositions = 0;
    int m_outOfViewAoiIn = 0;

    int m_localTileTransitions = 0;
    int m_remoteTileTransitions = 0;
    int m_localGridTransitions = 0;
    int m_remoteGridTransitions = 0;
    int m_gridContinuitySamples = 0;
    int m_completedGridMoveSessions = 0;
    int m_gridGapFailures = 0;

    int m_positionChecks = 0;
    int m_positionFailures = 0;
    int m_localPositionFailures = 0;
    int m_remotePositionFailures = 0;
    int m_initialPositionChecks = 0;
    int m_initialPositionFailures = 0;
    int m_initialLogicChecks = 0;
    int m_initialLogicFailures = 0;
    int m_initialRenderChecks = 0;
    int m_initialRenderFailures = 0;
    int m_completedInitialSync = 0;
    int m_gridPositionFailures = 0;
    int m_logicErrors = 0;
    double m_maxPositionCorrection = 0.0;
    double m_maxGridSampleGapMs = 0.0;
};
