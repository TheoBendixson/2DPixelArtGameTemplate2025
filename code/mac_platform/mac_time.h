
real32
MacGetSecondsElapsed(mach_timebase_info_data_t *TimeBase, uint64 Start, uint64 End)
{
	uint64 Elapsed = (End - Start);
    real32 Result = (real32)(Elapsed * (TimeBase->numer / TimeBase->denom)) / 1000.f / 1000.f / 1000.f;
    return(Result);
}

inline uint64
MacGetMicrosecondsElapsed(mach_timebase_info_data_t *TimeBase, uint64 Start, uint64 End)
{
    uint64 Elapsed = (End - Start);
    uint64 NanosecondsForFrame = (Elapsed * (TimeBase->numer / TimeBase->denom));
    uint64 MicrosecondsForFrame = NanosecondsForFrame/1000;
    return (MicrosecondsForFrame);
}
