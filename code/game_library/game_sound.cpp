
void
SaveVolumeSettingIfChanged(game_memory *Memory, game_state *GameState)
{
    if (GameState->VolumeChanged)
    {
        game_settings_file_header SettingsFile = {};
        SettingsFile.MusicVolume = GameState->AudioState.MusicVolume;
        SettingsFile.SFXVolume = GameState->AudioState.SFXVolume;
        char *Filename = "settings.msf";
        b32 Written = Memory->PlatformWriteEntireFile(Filename, sizeof(game_settings_file_header), &SettingsFile);
    }
}

f32 *
GetVolumeHandle(game_state *GameState, audio_settings_option Option)
{
    f32 *VolumeHandle = NULL;

    if (Option == AudioSettingsOptionMusicVolume)
    {
        VolumeHandle = &GameState->AudioState.MusicVolume;
    } else if (Option == AudioSettingsOptionSFXVolume)
    {
        VolumeHandle = &GameState->AudioState.SFXVolume;
    }

    return VolumeHandle;
}

// Returns 0 if all slots are used.
playing_sound *
PlaySound(game_audio_state *AudioState, loaded_sound_id SoundId,
	      f32 Volume = 1.f, f32 Pitch = 1.f, u64 EntityID = 0)
{
    Assert((s32)SoundId >= 0);

    playing_sound *Result = 0;
		
    for (s32 i = 0; i < AudioState->MaxPlayingSounds; i++)
    {
        if (!AudioState->PlayingSounds[i].Occupied)
        {
            Result = &AudioState->PlayingSounds[i];
            break;
        }
    }

    if (Result)
    {
        ZeroStruct(Result);

        f32 SourceVolume = Volume;
        Volume = SourceVolume;

        Result->Occupied = true;
        Result->Id = ++AudioState->LastGivenId;
        Result->SoundId = SoundId;

        Result->StartedPlaying = true;
        Result->Volume[0] = Volume;
        Result->Volume[1] = Volume;
        Result->VolumeTarget[0] = Volume;
        Result->VolumeTarget[1] = Volume;

        Result->Pitch = Pitch;
        Result->PitchTarget = Pitch;
        Result->EntityID = EntityID;
    }

    return Result;
}

void
SoundChangeVolumeByTime(playing_sound *Sound, f32 Volume, f32 TimeInSeconds = .1f)
{
    Sound->VolumeTarget[0] = Volume;
    Sound->VolumeTarget[1] = Volume;
    f32 MinDelta = .0001f;
    Sound->dVolume[0] = Max(MinDelta, SafeDivide(Abs(Sound->Volume[0] - Volume), TimeInSeconds, 10.f));
    Sound->dVolume[1] = Max(MinDelta, SafeDivide(Abs(Sound->Volume[1] - Volume), TimeInSeconds, 10.f));
}

void
SoundChangeVolumeByDelta(playing_sound *Sound, f32 Volume, f32 DeltaPerSecond = 10.f)
{
    Sound->VolumeTarget[0] = Volume;
    Sound->VolumeTarget[1] = Volume;
    Sound->dVolume[0] = DeltaPerSecond;
    Sound->dVolume[1] = DeltaPerSecond;
}

void
SoundSetPitch(playing_sound *Sound, f32 Pitch)
{
    Sound->Pitch = Sound->PitchTarget = Pitch;
}

void
FadeOutAndKillSound(playing_sound *Sound)
{
    f32 TimeToGoFromMaxTo0 = 0.1f;
    SoundChangeVolumeByDelta(Sound, 0, 1.f/TimeToGoFromMaxTo0);
    Sound->KillWhenVolumeReachesZero = true;
}

void
FadeOutAndKillAllOtherSoundsOfSameType(game_audio_state *AudioState, playing_sound *Sound)
{
    for (s32 i = 0; i < AudioState->MaxPlayingSounds; i++)
    {
        playing_sound *It = &AudioState->PlayingSounds[i];

        if (It->Occupied && It != Sound && It->SoundId == Sound->SoundId)
        {
            FadeOutAndKillSound(It);
        }
    }
}

playing_sound *
FindSound(game_audio_state *AudioState, u64 PlayingSoundId)
{
    playing_sound *Result = 0;

    for (s32 i = 0; i < AudioState->MaxPlayingSounds; i++)
    {
        playing_sound *It = &AudioState->PlayingSounds[i];
        if (It->Occupied && It->Id == PlayingSoundId)
        {
            Result = It;
            break;
        }
    }

    return Result;
}


playing_sound *
FindSoundByEntityID(game_state *GameState, u64 EntityID)
{
	game_audio_state *AudioState = &GameState->AudioState;
	playing_sound *Result = 0;

	for (s32 i = 0; i < AudioState->MaxPlayingSounds; i++)
	{
		playing_sound *It = &AudioState->PlayingSounds[i];
		if (It->Occupied && 
            It->EntityID == EntityID)
		{
			Result = It;
			break;
		}
	}

	return Result;
}

playing_sound *
FindSoundByTypeAndEntityID(game_state *GameState, loaded_sound_id SoundId, 
                           u64 EntityID)
{
	game_audio_state *AudioState = &GameState->AudioState;
	playing_sound *Result = 0;

	for (s32 i = 0; i < AudioState->MaxPlayingSounds; i++)
	{
		playing_sound *It = &AudioState->PlayingSounds[i];
		if (It->Occupied && 
            It->SoundId == SoundId &&
            It->EntityID == EntityID)
		{
			Result = It;
			break;
		}
	}

	return Result;
}

playing_sound *
FindSoundByType(game_state *GameState, loaded_sound_id SoundId)
{
	game_audio_state *AudioState = &GameState->AudioState;
	playing_sound *Result = 0;

	for (s32 i = 0; i < AudioState->MaxPlayingSounds; i++)
	{
		playing_sound *It = &AudioState->PlayingSounds[i];
		if (It->Occupied && It->SoundId == SoundId)
		{
			Result = It;
			break;
		}
	}

	return Result;
}

void
DelaySoundStart(game_audio_state *AudioState, playing_sound *Sound, f32 Seconds)
{
    if (Sound->CurrentSample <= 0)
    {
        loaded_sound *LoadedSound = &AudioState->LoadedSounds[(s32)Sound->SoundId];
        if (LoadedSound)
        {
            Sound->CurrentSample -= (s32)(Seconds*AudioState->SamplesPerSecond + .5f);
        }
    }
}

// Returns the lowest time since an active instance of SoundId started playing, in seconds.
// Returns FLT_MAX if no sounds found.
f32 
TimeSinceSoundOfTypePlayed(game_state *GameState, loaded_sound_id SoundId)
{
	game_audio_state *AudioState = &GameState->AudioState;
	f32 Result = FLT_MAX;

	for (s32 i = 0; i < AudioState->MaxPlayingSounds; i++)
	{
		playing_sound *It = &AudioState->PlayingSounds[i];
		if (It->Occupied && It->SoundId == SoundId && !It->Loop)
		{
			f32 Time = Max(0, (f32)It->CurrentSample/(f32)AudioState->SamplesPerSecond);
			Result = Min(Result, Time);
		}
	}

	return Result;

}

void
KillBackgroundMusic(game_state *GameState, game_audio_state *AudioState)
{
    playing_sound *BackgroundMusic = FindSound(AudioState, GameState->BackgroundMusicInstanceId);

    if (BackgroundMusic)
    {
        FadeOutAndKillSound(BackgroundMusic);
        GameState->BackgroundMusicInstanceId = 0;
    }
}

extern "C" 
GAME_GET_SOUND_SAMPLES(GameGetSoundSamples) 
{
    game_state *GameState = (game_state *)Memory->PermanentStoragePartition.Start;

    if (!GameState->SoundAndGraphicsLoaded)
    {
        return;
    }

    game_audio_state *AudioState = &GameState->AudioState;

	// PERF:   Performance: All this could be optimized with SIMD and more specialized code.

	// LOOKINTO:   Separate sfx and music controls:
	//             If we wanted separate music and sound effects controls in the settings menu,
	//             we could create two float sum buffers, have each sound output to one or the
	//             other depeding on its type, and at the end mix those two into the output buffer.
	f32 *FloatSumBuffer = (f32 *)Memory->TransientStoragePartition.Start;
	u64 FloatSumBufferSize = Memory->TransientStoragePartition.Size;

	s32 SamplesToAdvanceBeforeWriting = SoundOutputBuffer->SamplesToAdvanceBeforeWriting;
	s32 SamplesToWriteAndAdvance      = SoundOutputBuffer->SamplesToWriteAndAdvance;
	s32 SamplesToWriteExtra           = SoundOutputBuffer->SamplesToWriteExtra;
	s32 TotalSamplesToWrite = SamplesToWriteAndAdvance + SamplesToWriteExtra;

	// NOTE:   The complexity of this function is multiplied by the fact that it needs to do 3
	//         different things in order to prevent and recover from lag glitches:
	//            - Advance sounds by SamplesToAdvanceBeforeWriting without writing to the buffer.
	//            - Advance sounds by SamplesToWriteAndAdvance writing to the buffer.
	//            - Write SamplesToWriteExtra without advancing.
	// 
	//            All mixer features need to support these 3 things in a seamless manner, e.g.
	//            the platform layer should be able to write 1000 extra samples, and in the next
	//            frame advance its write cursor by 300 samples, and call this function telling it
	//            to advance 300 samples without writing, and it should sound seamless.
	//

	Assert(AudioState->SamplesPerSecond == SoundOutputBuffer->SamplesPerSecond);

    u32 MaxSamples = SoundOutputBuffer->SampleArrayCount/2;
    u32 BoundedSamplesToWrite = TotalSamplesToWrite;

    if (TotalSamplesToWrite > MaxSamples)
    {
        BoundedSamplesToWrite = MaxSamples;
    }

	Assert(FloatSumBufferSize >= BoundedSamplesToWrite*2*sizeof(f32));
	memset(FloatSumBuffer, 0, BoundedSamplesToWrite*2*sizeof(f32));


// MARK: Mix sounds and advance them.
	// Write sounds to float sum buffer
	for (s32 SoundIndex = 0; SoundIndex < AudioState->MaxPlayingSounds; SoundIndex++)
	{
		playing_sound *Sound = &AudioState->PlayingSounds[SoundIndex];

		if (!Sound->Occupied)
			continue;
		
		// NOTE:   A sound cannot start in the extra samples nor in the "advance before
		// writing" samples.
		if (!Sound->HasStarted && SamplesToWriteAndAdvance == 0)
			continue;

		s32 LocalSamplesToAdvanceBeforeWriting = SamplesToAdvanceBeforeWriting;
		s32 LocalSamplesToWriteAndAdvance = SamplesToWriteAndAdvance;
		s32 LocalSamplesToWriteExtra = SamplesToWriteExtra;
		f32 *Dest = FloatSumBuffer;

// MARK: Sound delay
		if (Sound->CurrentSample < 0)
		{
			Assert(!Sound->HasStarted);
			s32 SampleAfterAdvancing = Sound->CurrentSample + LocalSamplesToAdvanceBeforeWriting +
								       LocalSamplesToWriteAndAdvance;

			if (SampleAfterAdvancing < 0)
			{
				// After advancing, we're still in the delay zone, so we won't write any samples.
				Sound->CurrentSample = SampleAfterAdvancing;
				continue;
			}else{
				// We can finish off all delay with "advance before writing" samples and 
				// "write and advance" samples

				// Spend "advance before writing" samples on advancing the delay.
				s32 SpentSamples = MIN(-Sound->CurrentSample, LocalSamplesToAdvanceBeforeWriting);
				LocalSamplesToAdvanceBeforeWriting -= SpentSamples;
				Sound->CurrentSample += SpentSamples;

				if (Sound->CurrentSample < 0) // All "advance before writing" samples were applied.
				{
					// Spend "write and advance" samples on delay
					SpentSamples = MIN(-Sound->CurrentSample, LocalSamplesToWriteAndAdvance);
					LocalSamplesToWriteAndAdvance -= SpentSamples;
					Sound->CurrentSample += SpentSamples;

					Dest += 2*SpentSamples;
				}
				Assert(Sound->CurrentSample == 0);
			}
		}
		Assert(Sound->CurrentSample >= 0);

		Sound->HasStarted = true;
		
		loaded_sound *LoadedSound = &AudioState->LoadedSounds[(s32)Sound->SoundId];
		if (!LoadedSound->Samples)
		{ // Not loaded.
			if (!Sound->Loop)
			{
				ZeroStruct(Sound); // Kill sound
			}
			continue;
		}

		s32 SourceNumChannels = LoadedSound->NumChannels;
		s32 SourceIsStereo = (SourceNumChannels == 2 ? 1 : 0);
		s32 SourceNumSamples = LoadedSound->NumSamples;
		s16 *SourceSamples = LoadedSound->Samples;
		s16 *Source = LoadedSound->Samples + (Sound->CurrentSample*SourceNumChannels);

		Assert(SourceIsStereo); 

		b32 ConstantPitch = (Sound->PitchTarget == Sound->Pitch);
		b32 ConstantVolume = ((Sound->VolumeTarget[0] == Sound->Volume[0] &&
							   Sound->VolumeTarget[1] == Sound->Volume[1]) ||
							  (Sound->dVolume[0] == 0 && Sound->dVolume[1] == 0));

		Assert(SourceNumSamples);
		Assert(Sound->CurrentSample <= SourceNumSamples);
		
		// NOTE:   Here come all the specialized loops for mixing with different settings.

		if (ConstantPitch && ConstantVolume && Sound->Pitch == 1.f)
		{
			f32 Volume[2] = { Sound->Volume[0], Sound->Volume[1] };
			if (!Sound->Loop)
			{
// MARK: Constant Volume, Normal Pitch, No Loop
				// Advance without writing
				Sound->CurrentSample = MIN(Sound->CurrentSample + LocalSamplesToAdvanceBeforeWriting,
										   SourceNumSamples);

				// Write and advance
				int SamplesToWrite = MIN(LocalSamplesToWriteAndAdvance + LocalSamplesToWriteExtra,
									     SourceNumSamples - Sound->CurrentSample);
				if (SourceIsStereo)
				{
					for (s32 i = 0; i < SamplesToWrite; i++)
					{
						Dest[0] += Source[0]*Volume[0]; // L
						Dest[1] += Source[1]*Volume[1]; // R
						Dest += 2;
						Source += 2;
					}
				}else{
					for (s32 i = 0; i < SamplesToWrite; i++)
					{
						f32 Sample = (f32)*Source++;
						Dest[0] += Sample*Volume[0]; // L
						Dest[1] += Sample*Volume[1]; // R
						Dest += 2;
					}
				}

				Sound->CurrentSample += LocalSamplesToWriteAndAdvance;
			}else{
// MARK: Constant Volume, Normal Pitch, Loop
				// Advance without writing
				Sound->CurrentSample = (Sound->CurrentSample + LocalSamplesToAdvanceBeforeWriting) % SourceNumSamples;

				// Write and advance
				s32 SamplesToWrite = LocalSamplesToWriteAndAdvance + LocalSamplesToWriteExtra;
				s32 LocalCurrentSample = Sound->CurrentSample;
				while (SamplesToWrite)
				{
					s32 LocalSamplesToWrite = MIN(SamplesToWrite, SourceNumSamples - LocalCurrentSample);

					for (s32 i = 0; i < LocalSamplesToWrite; i++)
					{
						*Dest++ += ((f32)*Source)*Volume[0]; // L
						Source += SourceIsStereo;
						*Dest++ += ((f32)*Source)*Volume[1]; // R
						Source++;
					}

					SamplesToWrite -= LocalSamplesToWrite;
					LocalCurrentSample += LocalSamplesToWrite;
					if (LocalCurrentSample == SourceNumSamples)
					{
						LocalCurrentSample = 0;
						Source = LoadedSound->Samples;
					}
				}

				Sound->CurrentSample = (Sound->CurrentSample + LocalSamplesToWriteAndAdvance) % SourceNumSamples;
			}
			Sound->CurrentSampleFrac = 0;

		}else if (ConstantPitch && Sound->Pitch == 1.f)
		{
// MARK: Dynamic Volume, Normal Pitch (Loop & No Loop)
			f32 VolumeTarget[2] = { Sound->VolumeTarget[0], Sound->VolumeTarget[1] };

			// Advance without writing
			if (Sound->Loop){
				Sound->CurrentSample = (Sound->CurrentSample + LocalSamplesToAdvanceBeforeWriting) % SourceNumSamples;
			}else{
				Sound->CurrentSample = MIN(Sound->CurrentSample + LocalSamplesToAdvanceBeforeWriting, SourceNumSamples);
			}

			f32 SamplesTillVolumeReachesTarget[2];

			for(s32 i = 0; i < 2; i++)
			{
				f32 dVolumeInSamples = Sound->dVolume[i]/(f32)SoundOutputBuffer->SamplesPerSecond;
				if (dVolumeInSamples == 0)
				{
					// NOTE: (Pere)  Almost no delta, so we'll force it to keep still, which will also avoid division by 0.
					VolumeTarget[i] = Sound->Volume[i];
					SamplesTillVolumeReachesTarget[i] = 1.f;
					continue;
				}

				SamplesTillVolumeReachesTarget[i] = 
                    Max(1.f, Abs(Sound->Volume[i] - Sound->VolumeTarget[i])/dVolumeInSamples);

				// NOTE: (Pere)  Advance volume by "samples to advance without writing"
				Sound->Volume[i] = 
                    LerpClamp(Sound->Volume[i], Sound->VolumeTarget[i], 
                              (f32)LocalSamplesToAdvanceBeforeWriting/SamplesTillVolumeReachesTarget[i]);

				SamplesTillVolumeReachesTarget[i] = 
                    Max(1.f, Abs(Sound->Volume[i] - Sound->VolumeTarget[i])/dVolumeInSamples);
			}


			// Write and advance
			s32 SamplesToWrite = LocalSamplesToWriteAndAdvance + LocalSamplesToWriteExtra;
			s32 LocalCurrentSample = Sound->CurrentSample;

			f32 Volume[2] = { Sound->Volume[0], Sound->Volume[1] };

			s32 SamplesWritten = 0;
			while (SamplesToWrite)
			{
				s32 LocalSamplesToWrite = MIN(SamplesToWrite, SourceNumSamples - LocalCurrentSample);
	
				for (s32 i = 0; i < LocalSamplesToWrite; i++)
				{
					f32 CurrentVolume0 = 
                        LerpClamp(Volume[0], VolumeTarget[0], (f32)(i + SamplesWritten)/SamplesTillVolumeReachesTarget[0]);
					f32 CurrentVolume1 = 
                        LerpClamp(Volume[1], VolumeTarget[1], (f32)(i + SamplesWritten)/SamplesTillVolumeReachesTarget[1]);

					*Dest++ += ((f32)*Source)*CurrentVolume0; // L
					Source += SourceIsStereo;
					*Dest++ += ((f32)*Source)*CurrentVolume1; // R
					Source++;
				}

				SamplesWritten += LocalSamplesToWrite;
				SamplesToWrite -= LocalSamplesToWrite;
				if (!Sound->Loop)
					break;
			}
			
			Sound->Volume[0] = 
                LerpClamp(Sound->Volume[0], VolumeTarget[0], 
                          (f32)LocalSamplesToWriteAndAdvance/SamplesTillVolumeReachesTarget[0]);
			Sound->Volume[1] = 
                LerpClamp(Sound->Volume[1], VolumeTarget[1], 
                          (f32)LocalSamplesToWriteAndAdvance/SamplesTillVolumeReachesTarget[1]);

			if (Sound->Loop)
			{
				Sound->CurrentSample = (Sound->CurrentSample + LocalSamplesToWriteAndAdvance) % SourceNumSamples;
			}else{
				Sound->CurrentSample += LocalSamplesToWriteAndAdvance;
			}

			Sound->CurrentSampleFrac = 0;

		}else if (ConstantVolume && ConstantPitch)
		{
// MARK: Constant Volume, Constant Custom Pitch (Loop & No Loop)
			Sound->Pitch       = Clamp(Sound->Pitch, MIN_PITCH, MAX_PITCH);
			Sound->PitchTarget = Clamp(Sound->PitchTarget, MIN_PITCH, MAX_PITCH);
			f32 Pitch = Sound->Pitch;

			// NOTE:   For code simplicity, pitched looped sounds wrap at the last sample,
			//         meaning we don't interpolate between the last and first samples, and so
			//         we lose 1 sample's worth of time every time we loop.
			//         All pitched sounds also lose the last sample, even if they don't loop.
			//         But it's OK because I don't think anyone can notice that.
            
			// LOOKINTO:   This could be solved by allocating 1 extra sample in all loaded sounds
			//             at the end, matching the first sample. But we have better things to do.

			// Advance without writing
			if (Sound->Loop)
			{
				if (Sound->CurrentSample >= SourceNumSamples - 1)
				{
					Sound->CurrentSample = 0;
				}

				s32 ToAdvance = LocalSamplesToAdvanceBeforeWriting;
				while (ToAdvance)
				{
					f32 SourceSamplesLeft = (f32)(SourceNumSamples - Sound->CurrentSample) - Sound->CurrentSampleFrac;
					s32 MaxSamplesToAdvance = 1 + (s32)(SourceSamplesLeft/Pitch); // (The 1 is the first sample at the curSample position)
					s32 SamplesToAdvance = MIN(ToAdvance, MaxSamplesToAdvance);

					f32 NewRelativeSample = (f32)SamplesToAdvance/Pitch + Sound->CurrentSampleFrac;
					Sound->CurrentSample += (s32)NewRelativeSample;
					Sound->CurrentSampleFrac = Frac(NewRelativeSample);
					if (Sound->CurrentSample >= SourceNumSamples - 1)
					{
						Sound->CurrentSample = 0;
					}

					ToAdvance -= SamplesToAdvance;
				}
			}else{
				f32 NewRelativeSample = (f32)LocalSamplesToAdvanceBeforeWriting/Pitch + Sound->CurrentSampleFrac;
				Sound->CurrentSample += (s32)NewRelativeSample;
				Sound->CurrentSampleFrac = Frac(NewRelativeSample);
				if (Sound->CurrentSample >= SourceNumSamples - 1)
				{
					Sound->CurrentSample = SourceNumSamples;
					Sound->CurrentSampleFrac = 0;

					LocalSamplesToWriteAndAdvance = 0;
					LocalSamplesToWriteExtra = 0;
				}
			}

			// Write and advance
			f32 Volume[2] = { Sound->Volume[0], Sound->Volume[1] };
			
			s32 LocalCurrentSample = Sound->CurrentSample;
			f32 LocalCurrentSampleFrac = Sound->CurrentSampleFrac;

			s32 SamplesToWrite = LocalSamplesToWriteAndAdvance + LocalSamplesToWriteExtra;
			s32 SamplesToAdvance = LocalSamplesToWriteAndAdvance;

			while (SamplesToWrite)
			{
				f32 Epsilon = .0001f;
				f32 SourceSamplesLeft = (f32)(SourceNumSamples - LocalCurrentSample) - LocalCurrentSampleFrac - Epsilon;
				s32 MaxSamplesToWrite = 1 + (s32)(SourceSamplesLeft/Pitch); // (The 1 is the first sample at the curSample position)
				s32 LocalSamplesToWrite = MIN(SamplesToWrite, MaxSamplesToWrite);
				s32 LocalSamplesToAdvance = MIN(SamplesToAdvance, MaxSamplesToWrite);

				s16 *LocalSource = SourceSamples + SourceNumChannels*LocalCurrentSample;
				
				f32 Offset0 = LocalCurrentSampleFrac;
				f32 Offset1 = LocalCurrentSampleFrac + (f32)(LocalSamplesToWrite - 1)*Pitch;
				f32 Increment = SafeDivide0(1.f, (f32)(LocalSamplesToWrite - 1));

				for (s32 i = 0; i < LocalSamplesToWrite; i++)
				{
					f32 Offset = Lerp(Offset0, Offset1, (f32)i*Increment);
					f32 SampleFrac = Frac(Offset);

					s16 *Sample = LocalSource + SourceNumChannels*(s32)Offset;
					s16 *NextSample = Sample + SourceNumChannels;

					*Dest++ += Lerp((f32)*Sample, (f32)*NextSample, SampleFrac)*Volume[0];
					Sample += SourceIsStereo;
					NextSample += SourceIsStereo;
					*Dest++ += Lerp((f32)*Sample, (f32)*NextSample, SampleFrac)*Volume[1];
				}

				SamplesToWrite -= LocalSamplesToWrite;
				SamplesToAdvance -= LocalSamplesToAdvance;


				// Advance Current Sample
				f32 AdvanceOffset = LocalCurrentSampleFrac + (f32)(LocalSamplesToAdvance)*Pitch;
				Sound->CurrentSample += (s32)AdvanceOffset;
				Sound->CurrentSampleFrac = Frac(AdvanceOffset);

				f32 LocalAdvanceOffset = Offset1 + Pitch;
				LocalCurrentSample += (s32)LocalAdvanceOffset;
				LocalCurrentSampleFrac = Frac(LocalAdvanceOffset);

				if (Sound->Loop)
				{
					Sound->CurrentSample %= (SourceNumSamples - 1);
					LocalCurrentSample   %= (SourceNumSamples - 1);

				}else if (Sound->CurrentSample >= SourceNumSamples - 1)
				{
					Sound->CurrentSample = SourceNumSamples; // NOTE:   This ensures it will be deleted.
					Sound->CurrentSampleFrac = 0;
				}

				if (!Sound->Loop)
					break;
			}

		}else{
// MARK: Dynamic Pitch (Constant & Dynamic Volume, Loop & No Loop)
			// NOTE:   In this case I sacrificed some performance for code simplicity.

			// NOTE:   Here too, the last sample is skipped for code simplicity.

			Sound->Pitch       = Clamp(Sound->Pitch, MIN_PITCH, MAX_PITCH);
			Sound->PitchTarget = Clamp(Sound->PitchTarget, MIN_PITCH, MAX_PITCH);
			
			f32 Pitch = Sound->Pitch;
			f32 PitchTarget = Sound->PitchTarget;
			f32 Volume[2] = { Sound->Volume[0], Sound->Volume[1] };
			f32 VolumeTarget[2] = { Sound->VolumeTarget[0], Sound->VolumeTarget[1] };


			// Advance without writing
			f32 dVolumePerSample[2] = { Sound->dVolume[0]/SoundOutputBuffer->SamplesPerSecond,
										Sound->dVolume[1]/SoundOutputBuffer->SamplesPerSecond };
			f32 dPitchPerSample = Sound->dPitch/SoundOutputBuffer->SamplesPerSecond;

			if (0){
				f32 Offset = Sound->CurrentSampleFrac; // NOTE:   This is the new position relative to Sound->CurrentSample
				for (s32 i = 0; i < LocalSamplesToAdvanceBeforeWriting; i++)
				{
					Pitch = MoveValueTowards(Pitch, PitchTarget, dPitchPerSample);
					Offset += Pitch;
					Volume[0] = MoveValueTowards(Volume[0], VolumeTarget[0], dVolumePerSample[0]);
					Volume[1] = MoveValueTowards(Volume[1], VolumeTarget[1], dVolumePerSample[1]);
				}

				Sound->CurrentSample += (s32)Offset;
				Sound->CurrentSampleFrac = Frac(Offset);
				Sound->Pitch = Pitch;
				Sound->Volume[0] = Volume[0];
				Sound->Volume[1] = Volume[1];
			}

			if (Sound->Loop)
			{
				Sound->CurrentSample %= (SourceNumSamples - 1);
			}else if (Sound->CurrentSample >= SourceNumSamples - 1)
			{
				// NOTE:   Sound finished. Will be deleted.
				Sound->CurrentSample = SourceNumSamples;
				Sound->CurrentSampleFrac = 0;
			}


			// Write and advance
			s32 LocalCurrentSample = Sound->CurrentSample;
			f32 LocalCurrentSampleFrac = Sound->CurrentSampleFrac;

			s32 ToWriteAndAdvance = LocalSamplesToWriteAndAdvance;
			s32 ToWriteExtra = LocalSamplesToWriteExtra;

			while (ToWriteAndAdvance || ToWriteExtra)
			{
				// NOTE:   First we write the non-extra samples and advance the sound.
				//         Then we write the extra samples without advancing the sound.

				s32 ToWrite = (ToWriteAndAdvance ? ToWriteAndAdvance : ToWriteExtra);

				f32 Offset = LocalCurrentSampleFrac;

				s16 *SourceLimit = SourceSamples + SourceNumChannels*(SourceNumSamples - 1);
				s16 *SourceCurrent = SourceSamples + SourceNumChannels*LocalCurrentSample;
				while (ToWrite)
				{
					s16 *Sample = SourceCurrent + SourceNumChannels*(s32)Offset;
					if (Sample >= SourceLimit)
						break;
					s16 *NextSample = Sample + SourceNumChannels;

					f32 SampleFrac = Frac(Offset);

					*Dest++ += Lerp((f32)*Sample, (f32)*NextSample, SampleFrac)*Volume[0];
					Sample += SourceIsStereo;
					NextSample += SourceIsStereo;
					*Dest++ += Lerp((f32)*Sample, (f32)*NextSample, SampleFrac)*Volume[1];

					Offset += Pitch;
					Pitch = MoveValueTowards(Pitch, PitchTarget, dPitchPerSample);
					Volume[0] = MoveValueTowards(Volume[0], VolumeTarget[0], dVolumePerSample[0]);
					Volume[1] = MoveValueTowards(Volume[1], VolumeTarget[1], dVolumePerSample[1]);

					ToWrite--;
				}

				LocalCurrentSample += (s32)Offset;
				LocalCurrentSampleFrac = Frac(Offset);

				if (LocalCurrentSample >= SourceNumSamples - 1) // Reached end
				{
					if (Sound->Loop)
					{
						LocalCurrentSample %= SourceNumSamples - 1; // Wrap
					}else{
						LocalCurrentSample = SourceNumSamples; // NOTE: This will cause it to be killed.
						LocalCurrentSampleFrac = 0;
					}
				}

				if (ToWriteAndAdvance)
				{
					Sound->CurrentSample = LocalCurrentSample;
					Sound->CurrentSampleFrac = LocalCurrentSampleFrac;
					Sound->Pitch = Pitch;
					Sound->Volume[0] = Volume[0];
					Sound->Volume[1] = Volume[1];

					ToWriteAndAdvance = ToWrite;

				}else{
					ToWriteExtra = ToWrite;
				}

				// (If no-loop reached end of sound, leave)
				if (!Sound->Loop && LocalCurrentSample == SourceNumSamples - 1)
					break;
			}
		}


// MARK: Finish sounds
		b32 Kill = (!Sound->Loop && Sound->CurrentSample >= SourceNumSamples);
		if (Sound->KillWhenVolumeReachesZero &&
			Sound->Volume[0] == 0 && Sound->VolumeTarget[0] == 0 &&
			Sound->Volume[1] == 0 && Sound->VolumeTarget[1] == 0)
		{
			Kill = true;
		}
		if (Kill)
		{
			ZeroStruct(Sound);
		}
	}
	
	
#if 0
	// @DEBUG: Outputs a sine wave.
	{
		static s32 T = 0;
		s32 Period = 100; // In samples
		f32 Amplitude = 10000.f;

		if (SoundOutputBuffer->SamplesToAdvanceBeforeWriting){
			T += SoundOutputBuffer->SamplesToAdvanceBeforeWriting;
			T %= Period;
		}

		s32 TPrev = T;

		for(u32 i = 0; i < TotalSamplesToWrite; i++){
			f32 TFloat = 2*PI*((f32)T/(f32)Period);
			T = (T + 1) % Period;
			f32 Sample = Amplitude*sinf(TFloat);

			FloatSumBuffer[i*2] = Sample;
			FloatSumBuffer[i*2 + 1] = Sample; 
		}
		T = (TPrev + SoundOutputBuffer->SamplesToWriteAndAdvance) % Period;
	}
#endif

// MARK: Copy from sum buffer to output buffer.
	
	// Pre Advance Master Gain
	f32 dMasterGain = 10.f; // (in gain per second)
	f32 MasterGain = AudioState->MasterGain;
	f32 MasterGainTarget = AudioState->MasterGainTarget;
	if (SoundOutputBuffer->SamplesToAdvanceBeforeWriting)
	{
		f32 SecondsToAdvance = SoundOutputBuffer->SamplesToAdvanceBeforeWriting/(f32)SoundOutputBuffer->SamplesPerSecond;
		f32 SecondsTillMasterGainReachesTarget = Abs(MasterGain - MasterGainTarget)/dMasterGain;
		MasterGain = LerpClamp(MasterGain, MasterGainTarget,
							   SecondsToAdvance/SecondsTillMasterGainReachesTarget);
	}

	f32 dMasterGainInSamples = dMasterGain/(f32)SoundOutputBuffer->SamplesPerSecond;
	f32 SamplesTillMasterGainReachesTarget = Abs(MasterGain - MasterGainTarget)/dMasterGainInSamples;
	if (SamplesTillMasterGainReachesTarget == 0)
		SamplesTillMasterGainReachesTarget = 1.f; // (Avoid division by 0)


	for (u32 i = 0; i < BoundedSamplesToWrite; i++)
	{
        u32 Channel1Index = i*2;
        u32 Channel2Index = (i*2) + 1;
		f32 CurrentMasterGain = LerpClamp(MasterGain, MasterGainTarget, (f32)i/SamplesTillMasterGainReachesTarget);
		f32 Sample0 = Clamp(CurrentMasterGain*FloatSumBuffer[Channel1Index],     -32768.f, 32767.f);
		f32 Sample1 = Clamp(CurrentMasterGain*FloatSumBuffer[Channel2Index], -32768.f, 32767.f);
        SoundOutputBuffer->Samples[Channel1Index] = (s16)Sample0;
        SoundOutputBuffer->Samples[Channel2Index] = (s16)Sample1;
    }

	AudioState->MasterGain = LerpClamp(MasterGain, MasterGainTarget, (f32)SoundOutputBuffer->SamplesToWriteAndAdvance/SamplesTillMasterGainReachesTarget);

}

b32
SoundEffectIsPlaying(game_audio_state *AudioState, loaded_sound_id SoundEffectID)
{
    for (s32 PlayingSoundIndex = 0;
         PlayingSoundIndex < AudioState->MaxPlayingSounds;
         PlayingSoundIndex += 1)
    {
        playing_sound *PlayingSound = &AudioState->PlayingSounds[PlayingSoundIndex];

        if (PlayingSound->Occupied &&
            PlayingSound->SoundId == SoundEffectID)
        {
            return true;
        }
    }

    return false;
}

