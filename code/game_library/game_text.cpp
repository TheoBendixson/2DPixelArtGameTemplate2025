
void 
Reverse(char* String, u32 Length)
{
    u32 Front = 0; 
    s32 Back = Length - 1; 
    u32 Temp = 0;

    while (Front < Back) 
    {
        Temp = String[Front];
        String[Front] = String[Back];
        String[Back] = Temp;
        Front++;
        Back--;
    }
}

u32 
IntegerToString(u32 Number, char *String, u32 DigitCount)
{
    u32 Index = 0;

    if (Number == 0)
    {
        String[Index++] = '0';
    } else
    {
        while (Number) 
        {
            String[Index++] = (Number % 10) + '0';
            Number = Number / 10;
        }
    }
 
    // If number of digits required is more, then
    // add 0s at the beginning
    while (Index < DigitCount)
    {
        String[Index++] = '0';
    } 

    Reverse(String, Index);
    String[Index] = '\0';
    return Index;
}

void 
FloatingToAlpha(r32 Number, char* Result, u32 AmountAfterDecimalPoint)
{
    u32 IntegerPart = (u32)Number;
    r32 FloatingPart = Number - (r32)IntegerPart;
 
    u32 Index = IntegerToString(IntegerPart, Result, 0);
 
    if (AmountAfterDecimalPoint != 0) 
    {
        Result[Index] = '.'; 
        FloatingPart = FloatingPart * (r32)pow(10, AmountAfterDecimalPoint);
        IntegerToString((u32)FloatingPart, Result + Index + 1, AmountAfterDecimalPoint);
    }
}
