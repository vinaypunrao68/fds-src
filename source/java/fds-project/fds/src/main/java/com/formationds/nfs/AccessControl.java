package com.formationds.nfs;

import org.dcache.acl.ACE;
import org.dcache.acl.enums.AceFlags;
import org.dcache.acl.enums.AceType;
import org.dcache.acl.enums.Who;
import org.dcache.auth.Subjects;
import org.dcache.nfs.status.BadOwnerException;
import org.dcache.nfs.v4.NfsIdMapping;
import org.dcache.nfs.v4.xdr.nfsace4;
import org.dcache.nfs.vfs.AclCheckable;

import javax.security.auth.Subject;
import java.util.List;

public class AccessControl {
    static final int ACE4_READ_DATA            = 0x00000001;
    static final int ACE4_LIST_DIRECTORY       = 0x00000001;
    static final int ACE4_WRITE_DATA           = 0x00000002;
    static final int ACE4_ADD_FILE             = 0x00000002;
    static final int ACE4_APPEND_DATA          = 0x00000004;
    static final int ACE4_ADD_SUBDIRECTORY     = 0x00000004;
    static final int ACE4_READ_NAMED_ATTRS     = 0x00000008;
    static final int ACE4_WRITE_NAMED_ATTRS    = 0x00000010;
    static final int ACE4_EXECUTE              = 0x00000020;
    static final int ACE4_DELETE_CHILD         = 0x00000040;
    static final int ACE4_READ_ATTRIBUTES      = 0x00000080;
    static final int ACE4_WRITE_ATTRIBUTES     = 0x00000100;
    static final int ACE4_DELETE               = 0x00010000;
    static final int ACE4_READ_ACL             = 0x00020000;
    static final int ACE4_WRITE_ACL            = 0x00040000;
    static final int ACE4_WRITE_OWNER          = 0x00080000;
    static final int ACE4_SYNCHRONIZE          = 0x00100000;

    public static AclCheckable.Access check(Subject subject, List<ACE> acl, int ownerUid, int ownerGid, int accessMask) {
        int validatedMask = 0;
        for(ACE ace : acl) {
            if(validatedMask == accessMask)
                break;

            int relevantAccessMask = ~validatedMask & accessMask & ace.getAccessMsk();
            if(relevantAccessMask == 0 || !doesAceApplyToSubject(subject, ace, ownerUid, ownerGid))
                continue;

            switch(ace.getType()) {
                case ACCESS_DENIED_ACE_TYPE:
                    return AclCheckable.Access.DENY;
                case ACCESS_ALLOWED_ACE_TYPE:
                    validatedMask |= relevantAccessMask;
                    break;
                case ACCESS_ALARM_ACE_TYPE:
                    // TODO: implement?
                    break;
                case ACCESS_AUDIT_ACE_TYPE:
                    // TODO: implement?
                    break;
            }
        }

        if(validatedMask == accessMask)
            return AclCheckable.Access.ALLOW;

        return AclCheckable.Access.UNDEFINED;
    }

    public static ACE makeAceFromNfsace4(nfsace4 ace, NfsIdMapping idMap) throws BadOwnerException {
        String name = ace.who.toString();
        Who who = Who.fromAbbreviation(name);
        if(who != null)
            return new ACE(AceType.valueOf(ace.type.value.value), ace.flag.value.value, ace.access_mask.value.value, who, -1, ACE.DEFAULT_ADDRESS_MSK);

        if(AceFlags.IDENTIFIER_GROUP.matches(ace.flag.value.value))
            return new ACE(AceType.valueOf(ace.type.value.value), ace.flag.value.value, ace.access_mask.value.value, Who.GROUP, idMap.principalToGid(name), ACE.DEFAULT_ADDRESS_MSK);

        return new ACE(AceType.valueOf(ace.type.value.value), ace.flag.value.value, ace.access_mask.value.value, Who.USER, idMap.principalToUid(name), ACE.DEFAULT_ADDRESS_MSK);
    }

    private static boolean doesAceApplyToSubject(Subject subject, ACE ace, int ownerUid, int ownerGid) {
        Who aceWho = ace.getWho();
        int aceWhoId = ace.getWhoID();

        return aceWho == Who.EVERYONE
                || (aceWho == Who.OWNER && Subjects.hasUid(subject, ownerUid))
                || (aceWho == Who.OWNER_GROUP && Subjects.hasGid(subject, ownerGid))
                || (aceWho == Who.USER && Subjects.hasUid(subject, aceWhoId))
                || (aceWho == Who.GROUP && Subjects.hasGid(subject, aceWhoId));
    }
}
