#include "cornet/cornet.h"

cn_addrmap *cn_makeAddrmap(const char *refname)
{
    cn_addrmap *result = malloc(sizeof(cn_addrmap));
    result->dics = malloc(sizeof(cn_addrmap *) * 1000);
    result->refname = refname;
    int i;
    for(i = 0; i < 10; i++)
    {
        result->dics[i] = cn_makeDict4b("cn_addrmap.dics");
    }
    return result;
}

static unsigned int getDicIndexFromSockAddr(cn_sockaddr *ip)
{
    unsigned int dicIndex = 0;
    int i;
    /*
    for(i = 1; i <= 10; i++)
    {
        if(ip->node1 < 26 * i)
        {
            dicIndex += i * 1000;
        }
    }
    for(i = 1; i <= 10; i++)
    {
        if(ip->node2 < 26 * i)
        {
            dicIndex += i * 100;
        }
    }
    for(i = 1; i <= 10; i++)
    {
        if(ip->node3 < 26 * i)
        {
            dicIndex += i * 10;
        }
    }
    for(i = 1; i <= 10; i++)
    {
        if(ip->node4 < 26 * i)
        {
            dicIndex += i * 1;
        }
    }
    */
    i = ip->node4 % 10;
    return dicIndex;
}


int cn_addrmapSet(cn_addrmap *taddrmap, cn_sockaddr *addr, void *object)
{
    // Defensive code
    if(taddrmap == NULL || addr == NULL){return -1;}

    // Calculate the dictionary index in dictionary array
    unsigned int dicIndx = getDicIndexFromSockAddr(addr);

    // Fetch the Port Connection Dictionary
    cn_dictionary4b *remoteHostPortConns = cn_typeGet(cn_dict4bGet(taddrmap->dics[dicIndx], addr->intIp), cn_dictionary4b_type_id);
    if(remoteHostPortConns == NULL)
    {
        // This is first port connection for this ip. lets create the port connection dictionary
        cn_dict4bSet(taddrmap->dics[dicIndx], addr->intIp, cn_makeDict4b("addrmap.portmap"));
    }
    // finnaly set our object to destined ip port map
    cn_dict4bSet(remoteHostPortConns, (uint32_t)addr->port, object);
    return 0;
}

void *cn_addrmapGet(cn_addrmap *taddrmap, cn_sockaddr *addr)
{
    // Defensive code
    if(taddrmap == NULL || addr == NULL){return NULL;}

    // Calculate the dictionary index in dictionary array
    unsigned int dicIndx = getDicIndexFromSockAddr(addr);

    // Fetch the Port Connection Dictionary
    cn_dictionary4b *remoteHostPortConns = cn_typeGet(cn_dict4bGet(taddrmap->dics[dicIndx], addr->intIp), cn_dictionary4b_type_id);
    // failed to fetch return null
    if(remoteHostPortConns == NULL){return NULL;}

    // return object on port connections dictionary
    return cn_dict4bGet(remoteHostPortConns, (uint32_t)addr->port);
}

int cn_addrmapRemove(cn_addrmap *taddrmap, cn_sockaddr *addr)
{
    // Defensive code
    if(taddrmap == NULL || addr == NULL){return -1;}

    // Calculate the dictionary index in dictionary array
    unsigned int dicIndx = getDicIndexFromSockAddr(addr);

    // Fetch the Port Connection Dictionary
    cn_dictionary4b *remoteHostPortConns = cn_typeGet(cn_dict4bGet(taddrmap->dics[dicIndx], addr->intIp), cn_dictionary4b_type_id);
    // failed to fetch return null
    if(remoteHostPortConns == NULL){return -1;}

    // remove connection object with port as key
    int n = cn_dict4bRemoveByKey(remoteHostPortConns, addr->port);

    if(remoteHostPortConns->cnt < 1)
    {
        // no other connection on this ip. lets remove the port connection dictionary and destroy it
        cn_dict4bRemoveByKey(taddrmap->dics[dicIndx], addr->intIp);
        cn_desDict4b(remoteHostPortConns);
    }
    return n;
}
