#define MAXARGSIZE 400
// Pipes ordering in pipefds : 
//       from Highest to Lowest: 
                // HighestRead :0 
                // HighestWrite:1     
                // HighLowRead :2     
                // HighLowWrite:3     
                // LowHighRead :4     
                // LowHighWrite:5     
                // LowestRead  :6     
                // LowestWrite :7     