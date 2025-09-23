
b32
TriggerAchievementIfNotAlreadyAchieved(achievement *Achievement)
{
    if (!Achievement->Achieved)
    {
        Achievement->Achieved = true;
        return true;
    }

    return false;
}

