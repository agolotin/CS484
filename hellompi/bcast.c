// Broadcast value to all nodes
Bcast(int numdim, int rank, float *value)
{
    int notparticipating = pow(2,numdim-1)-1;
    int bitmask = pow(2,numdim-1);
    float newvalue = *value;
    for(curdim = 0; curdim < numdim; curdim++) {
	if ((rank & notparticipating) == 0) {
	    if ((rank & bitmask) == 0) {
		int msg_dest = rank ^ bitmask;
		send(sum, msg_dest);
	    } else {
		int msg_src = rank ^ bitmask;
		recv(&newvalue, msg_src);
            }
        }
	notparticipating >>= 1;
        bitmask >>=1;
     }
    *value = newvalue;
}
