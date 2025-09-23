
enum simulated_device_type
{
    SimulatedDeviceType_None,
    SimulatedDeviceType_SteamDeckStandard,
    SimulatedDeviceType_iPhone11
};

struct device_simulator_settings
{
    simulated_device_type DeviceType;
    b32 UsesTouchControls;
    s32 WindowWidth;
    s32 WindowHeight;
};
