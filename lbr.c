/******************************************************************************
 *
 * LBR = lock-based reclamation. No deferred deletion is needed, so provide
 * dummy functions.
 *
 *****************************************************************************/

void mr_init()
{
    /* Do nothing. */
    return;
}

void mr_thread_exit()
{
    /* Do nothing. */
    return;
}

void mr_reinitialize()
{
    /* Do nothing. */
    return;
}
