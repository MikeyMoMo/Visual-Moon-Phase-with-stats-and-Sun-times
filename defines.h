#define RGB565(r,g,b) ((((r>>3)<<11) | ((g>>2)<<5) | (b>>3)))

#define iIN_RANGE(v, low, high) (((low) <= (high)) ? ((v) >= (low) && (v) <= (high)) : ((v) >= (low) || (v) <= (high)))
//SleepTime = xIN_RANGE(12, 23, 10);  // Should return false
#define xIN_RANGE(v, low, high) (((low) <= (high)) ? ((v) > (low) && (v) < (high)) : ((v) > (low) || (v) < (high)))

// Used to be routines but I figured out how to make it a macro.  Not so hard!
///***************************************************************************/
//bool iInRange (int v, int low, int high)  // Inclusive
///***************************************************************************/
//{
//  return (low <= high) ? (v >= low && v <= high) : (v >= low || v <= high);
//}
///***************************************************************************/
//bool xInRange (int v, int low, int high)  // Exclusive
///***************************************************************************/
//{
////SleepTime = xIN_RANGE(21, 23, 10);  // Should return false
//  return (low  <=  high)  ? ( v  >  low  &&  v  < high)   : ( v  >  low  ||  v  <  high);
//}
