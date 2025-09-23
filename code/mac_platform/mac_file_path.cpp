
char * ConvertAbsoluteURLToFileURL(NSURL *FileURL)
{
    NSMutableString *FilePath = [[FileURL absoluteString] mutableCopy];
    [FilePath replaceOccurrencesOfString: @"file://" 
                              withString: @""
                                 options: NSCaseInsensitiveSearch
                                   range: NSMakeRange(0,7)]; 
    char *LocalFilename = (char *)[FilePath cStringUsingEncoding: NSUTF8StringEncoding];
    [FilePath release];
    return (LocalFilename);

}
