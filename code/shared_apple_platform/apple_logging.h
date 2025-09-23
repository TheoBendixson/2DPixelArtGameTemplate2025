
PLATFORM_LOG_MESSAGE(PlatformLogMessage)
{
    NSString *LogMessage = [[NSString alloc] initWithCString: Message
                                                    encoding: NSUTF8StringEncoding];
    NSLog(@"%@", LogMessage);
    [LogMessage release];
}
