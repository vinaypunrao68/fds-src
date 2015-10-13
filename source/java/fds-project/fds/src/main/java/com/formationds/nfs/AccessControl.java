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
import java.util.ArrayList;
import java.util.List;

public class AccessControl {
    static final int MODE4_SUID = 0x800;  /* set user id on execution */
    static final int MODE4_SGID = 0x400;  /* set group id on execution */
    static final int MODE4_SVTX = 0x200;  /* save text even after use */
    static final int MODE4_RUSR = 0x100;  /* read permission: owner */
    static final int MODE4_WUSR = 0x080;  /* write permission: owner */
    static final int MODE4_XUSR = 0x040;  /* execute permission: owner */
    static final int MODE4_RGRP = 0x020;  /* read permission: group */
    static final int MODE4_WGRP = 0x010;  /* write permission: group */
    static final int MODE4_XGRP = 0x008;  /* execute permission: group */
    static final int MODE4_ROTH = 0x004;  /* read permission: other */
    static final int MODE4_WOTH = 0x002;  /* write permission: other */
    static final int MODE4_XOTH = 0x001;  /* execute permission: other */

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

    static final int ACE4_POSIX_READ_MASK = ACE4_READ_DATA | ACE4_LIST_DIRECTORY;
    static final int ACE4_POSIX_WRITE_MASK = ACE4_WRITE_DATA | ACE4_APPEND_DATA | ACE4_ADD_FILE | ACE4_ADD_SUBDIRECTORY | ACE4_DELETE_CHILD;
    static final int ACE4_POSIX_EXECUTE_MASK = ACE4_EXECUTE;
    static final int ACE4_POSIX_ALL = ACE4_POSIX_EXECUTE_MASK | ACE4_POSIX_READ_MASK | ACE4_POSIX_WRITE_MASK;

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

    private static MaskPair getFullAccessMask(List<ACE> acl, Who who) {
        MaskPair pair = new MaskPair();

        for(ACE ace : acl) {
            if(ace.getWho() != who && ace.getWho() != Who.EVERYONE)
                continue;

            int aceMask = ace.getAccessMsk();

            if(ace.getType() == AceType.ACCESS_ALLOWED_ACE_TYPE)
                pair.allow(aceMask);
            else if(ace.getType() == AceType.ACCESS_DENIED_ACE_TYPE)
                pair.deny(aceMask);
        }

        return pair;
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

    public static int updateModeAccess(int priorMode, int aclMode) {
        return (priorMode & ~ACE4_POSIX_ALL) | (aclMode & ACE4_POSIX_ALL);
    }

    public static int deriveMode(List<ACE> acl) {
        MaskPair everyone = getFullAccessMask(acl, Who.EVERYONE);
        MaskPair owner_group = getFullAccessMask(acl, Who.OWNER_GROUP);
        MaskPair owner_user = getFullAccessMask(acl, Who.OWNER);

        return (everyone.allows(ACE4_POSIX_READ_MASK) ? MODE4_ROTH : 0)
                | (everyone.allows(ACE4_POSIX_WRITE_MASK) ? MODE4_WOTH : 0)
                | (everyone.allows(ACE4_POSIX_EXECUTE_MASK) ? MODE4_XOTH : 0)
                | (owner_group.allows(ACE4_POSIX_READ_MASK) ? MODE4_RGRP : 0)
                | (owner_group.allows(ACE4_POSIX_WRITE_MASK) ? MODE4_WGRP : 0)
                | (owner_group.allows(ACE4_POSIX_EXECUTE_MASK) ? MODE4_XGRP : 0)
                | (owner_user.allows(ACE4_POSIX_READ_MASK) ? MODE4_RUSR : 0)
                | (owner_user.allows(ACE4_POSIX_WRITE_MASK) ? MODE4_WUSR : 0)
                | (owner_user.allows(ACE4_POSIX_EXECUTE_MASK) ? MODE4_XUSR : 0);
    }

    public static List<ACE> aclForMode(int mode) {
        int other = ((mode & MODE4_ROTH) != 0 ? ACE4_POSIX_READ_MASK : 0)
                | ((mode & MODE4_WOTH) != 0 ? ACE4_POSIX_WRITE_MASK : 0)
                | ((mode & MODE4_XOTH) != 0 ? ACE4_POSIX_EXECUTE_MASK : 0);

        int otherDeny = ~other & ACE4_POSIX_ALL;

        int ownerGroup = ((mode & MODE4_RGRP) != 0 ? ACE4_POSIX_READ_MASK : 0)
                | ((mode & MODE4_WGRP) != 0 ? ACE4_POSIX_WRITE_MASK : 0)
                | ((mode & MODE4_XGRP) != 0 ? ACE4_POSIX_EXECUTE_MASK : 0);

        int ownerGroupDeny = ~ownerGroup & ACE4_POSIX_ALL;

        int owner = ((mode & MODE4_RUSR) != 0 ? ACE4_POSIX_READ_MASK : 0)
                | ((mode & MODE4_WUSR) != 0 ? ACE4_POSIX_WRITE_MASK : 0)
                | ((mode & MODE4_XUSR) != 0 ? ACE4_POSIX_EXECUTE_MASK : 0);

        int ownerDeny = ~owner & ACE4_POSIX_ALL;

        ArrayList<ACE> aceList = new ArrayList<>(6);
        if(owner != 0)
            aceList.add(new ACE(AceType.ACCESS_ALLOWED_ACE_TYPE, 0, owner, Who.OWNER, -1, ACE.DEFAULT_ADDRESS_MSK));
        if(ownerDeny != 0)
            aceList.add(new ACE(AceType.ACCESS_DENIED_ACE_TYPE, 0, ownerDeny, Who.OWNER, -1, ACE.DEFAULT_ADDRESS_MSK));
        if(ownerGroup != 0)
            aceList.add(new ACE(AceType.ACCESS_ALLOWED_ACE_TYPE, 0, ownerGroup, Who.OWNER_GROUP, -1, ACE.DEFAULT_ADDRESS_MSK));
        if(ownerGroupDeny != 0)
            aceList.add(new ACE(AceType.ACCESS_DENIED_ACE_TYPE, 0, ownerGroupDeny, Who.OWNER_GROUP, -1, ACE.DEFAULT_ADDRESS_MSK));
        if(other != 0)
            aceList.add(new ACE(AceType.ACCESS_ALLOWED_ACE_TYPE, 0, other, Who.EVERYONE, -1, ACE.DEFAULT_ADDRESS_MSK));
        if(otherDeny != 0)
            aceList.add(new ACE(AceType.ACCESS_DENIED_ACE_TYPE, 0, otherDeny, Who.EVERYONE, -1, ACE.DEFAULT_ADDRESS_MSK));

        return aceList;
    }

    private static class MaskPair {
        int allowed;
        int denied;

        public void allow(int aceMask) {
            allowed |= (aceMask & ~denied);
        }

        public void deny(int aceMask) {
            denied |= (aceMask & ~allowed);
        }

        public boolean allows(int mask) {
            return (allowed & mask) == mask;
        }
    }
}
