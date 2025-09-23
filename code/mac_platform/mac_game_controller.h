void ControllerInput(void *context, IOReturn result, 
                     void *sender, IOHIDValueRef value)
{
    if(result != kIOReturnSuccess) 
    {
        return;
    }

    mac_game_controller *MacGameController = (mac_game_controller *)context;
    
    IOHIDElementRef Element = IOHIDValueGetElement(value);    
    u32 UsagePage = IOHIDElementGetUsagePage(Element);
    u32 Usage = IOHIDElementGetUsage(Element);

    //Buttons
    if(UsagePage == kHIDPage_Button) {

        b32 ButtonState = (b32)IOHIDValueGetIntegerValue(value);

        if (Usage == MacGameController->ButtonAUsageID) 
        { 
            MacGameController->ButtonAState = ButtonState; 
        } else if (Usage == MacGameController->ButtonBUsageID)
        { 
            MacGameController->ButtonBState = ButtonState; 
        } else if(Usage == MacGameController->ButtonXUsageID) 
        { 
            MacGameController->ButtonXState = ButtonState; 
        } else if(Usage == MacGameController->ButtonYUsageID) 
        { 
            MacGameController->ButtonYState = ButtonState; 
        } else if(Usage == MacGameController->ButtonLeftShoulder1UsageID) 
        { 
            MacGameController->ButtonLeftShoulder1State = ButtonState; 
        } else if (Usage == MacGameController->ButtonLeftShoulder2UsageID)
        {
            MacGameController->ButtonLeftShoulder2State = ButtonState; 
        }
        else if(Usage == MacGameController->ButtonRightShoulder1UsageID) 
        { 
            MacGameController->ButtonRightShoulder1State = ButtonState; 
        } else if (Usage == MacGameController->ButtonRightShoulder2UsageID)
        {
            MacGameController->ButtonRightShoulder2State = ButtonState; 
        } else if (Usage == MacGameController->ButtonStartUsageID)
        {
            MacGameController->ButtonStartState = ButtonState;
        }
        else if (Usage == MacGameController->ButtonSelectUsageID)
        {
            MacGameController->ButtonSelectState = ButtonState;
        }
    }

    //dPad
    else if(UsagePage == kHIDPage_GenericDesktop) {

        double_t Analog = IOHIDValueGetScaledValue(value, kIOHIDValueScaleTypeCalibrated);
        
        if (Usage == MacGameController->LeftThumbXUsageID) {
            MacGameController->LeftThumbstickX = (real32)Analog;
        }

        if (Usage == MacGameController->LeftThumbYUsageID) {
            MacGameController->LeftThumbstickY = (real32)Analog;
        }

        if(Usage == kHIDUsage_GD_Hatswitch) { 
            int DPadState = (int)IOHIDValueGetIntegerValue(value);
            s32 DPadX = 0;
            s32 DPadY = 0;

            switch(DPadState) {
                case 0: DPadX = 0; DPadY = 1; break;
                case 1: DPadX = 1; DPadY = 1; break;
                case 2: DPadX = 1; DPadY = 0; break;
                case 3: DPadX = 1; DPadY = -1; break;
                case 4: DPadX = 0; DPadY = -1; break;
                case 5: DPadX = -1; DPadY = -1; break;
                case 6: DPadX = -1; DPadY = 0; break;
                case 7: DPadX = -1; DPadY = 1; break;
                default: DPadX = 0; DPadY = 0; break;
            }

            MacGameController->DPadX = DPadX;
            MacGameController->DPadY = DPadY;
        }
    }
}

void ControllerConnected(void *context, IOReturn result, 
                         void *sender, IOHIDDeviceRef device)
{
    if(result != kIOReturnSuccess) {
        return;
    }

    NSUInteger vendorID = [(__bridge NSNumber *)IOHIDDeviceGetProperty(device, 
                                                                       CFSTR(kIOHIDVendorIDKey)) unsignedIntegerValue];
    NSUInteger productID = [(__bridge NSNumber *)IOHIDDeviceGetProperty(device, 
                                                                        CFSTR(kIOHIDProductIDKey)) unsignedIntegerValue];

    mac_game_controller *MacGameController = (mac_game_controller *)context;

    //if(vendorID == 0x054C && productID == 0x5C4) 
    //{
        NSLog(@"Sony Dualshock 4 detected.");

        //  Left Thumb Stick       
        MacGameController->LeftThumbXUsageID = kHIDUsage_GD_X;
        MacGameController->LeftThumbYUsageID = kHIDUsage_GD_Y;
        MacGameController->UsesHatSwitch = true;
 
        MacGameController->ButtonAUsageID = 0x02;
        MacGameController->ButtonBUsageID = 0x03;
        MacGameController->ButtonXUsageID = 0x01;
        MacGameController->ButtonYUsageID = 0x04;

        MacGameController->ButtonLeftShoulder1UsageID = 0x05;
        MacGameController->ButtonLeftShoulder2UsageID = 0x07;

        MacGameController->ButtonRightShoulder1UsageID = 0x06;
        MacGameController->ButtonRightShoulder2UsageID = 0x08;

        MacGameController->ButtonStartUsageID = 0x0a;
        MacGameController->ButtonSelectUsageID = 0x09;
    //}
    
    MacGameController->LeftThumbstickX = 128.0f;
    MacGameController->LeftThumbstickY = 128.0f;

    IOHIDDeviceRegisterInputValueCallback(device, ControllerInput, (void *)MacGameController);  
    IOHIDDeviceSetInputValueMatchingMultiple(device, (__bridge CFArrayRef)@[
        @{@(kIOHIDElementUsagePageKey): @(kHIDPage_GenericDesktop)},
        @{@(kIOHIDElementUsagePageKey): @(kHIDPage_Button)},
    ]);
}

void MacInitGameControllers(mac_game_controller *MacGameController) 
{
    IOHIDManagerRef HIDManager = IOHIDManagerCreate(kCFAllocatorDefault, 0);

    if (IOHIDManagerOpen(HIDManager, kIOHIDOptionsTypeNone) != kIOReturnSuccess) {
        NSLog(@"Error Initializing OSX Handmade Controllers");
        return;
    }

    IOHIDManagerRegisterDeviceMatchingCallback(HIDManager, ControllerConnected, (void *)MacGameController);

    IOHIDManagerSetDeviceMatchingMultiple(HIDManager, (__bridge CFArrayRef)@[
        @{@(kIOHIDDeviceUsagePageKey): @(kHIDPage_GenericDesktop), @(kIOHIDDeviceUsageKey): @(kHIDUsage_GD_GamePad)},
        @{@(kIOHIDDeviceUsagePageKey): @(kHIDPage_GenericDesktop), @(kIOHIDDeviceUsageKey): @(kHIDUsage_GD_MultiAxisController)},
    ]);
  
	IOHIDManagerScheduleWithRunLoop(HIDManager, 
                                    CFRunLoopGetMain(), 
                                    kCFRunLoopDefaultMode);
}

