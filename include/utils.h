#pragma once

int get_current_hour()
{
    return millis() / 3.6E6;
}
int get_current_minute()
{
    return (millis() / 60000) % 60;
}
int get_current_second()
{
    return (millis() / 1000) % 60;
}