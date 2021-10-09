#pragma once

template <typename T>
static T Limit(T in, T min, T max)
{
  if (in < min)
  {
    return min;
  }

  if (in > max)
  {
    return max;
  }

  return in;
}
