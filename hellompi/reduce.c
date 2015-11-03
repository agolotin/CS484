// Reduce values to one node
float ReduceSum(int numdim, int rank, float value)
{
    int notparticipating = 0;
    int bitmask = 1;
    float sum = value;
    float newvalue;
    for(curdim = 0; curdim < numdim; curdim++) {
	if ((rank & notparticipating) == 0) {
	    if ((rank & bitmask) != 0) {
		int msg_dest = rank ^ bitmask;
		send(sum, msg_dest);
	    } else {
		int msg_src = rank ^ bitmask;
		recv(&newvalue, msg_src);
                sum += newvalue;
            }
        }
	notparticipating = notparticipating ^ bitmask;
        bitmask <<=1;
    }
    return(sum);
}
