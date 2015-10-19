package com.formationds.nfs;

import org.dcache.acl.ACE;
import org.dcache.acl.enums.AceType;
import org.dcache.acl.enums.Who;
import org.dcache.auth.Subjects;
import org.dcache.nfs.vfs.AclCheckable;
import org.junit.Test;

import javax.security.auth.Subject;

import java.util.ArrayList;
import java.util.List;

import static org.junit.Assert.*;

public class AccessControlTest {
    @Test
    public void testCheck() {
        int uid1 = 10;
        int uid2 = 11;

        int gid1 = 20;
        int gid2 = 21;
        int gid3 = 22;
        int gid4 = 23;
        Subject subject1 = Subjects.of(uid1, gid1, gid2);
        Subject subject2 = Subjects.of(uid2, gid1, gid3);

        List<ACE> ownerAcl = new ArrayList<>();
        // only the owner can append
        ownerAcl.add(new ACE(AceType.ACCESS_ALLOWED_ACE_TYPE, 0, AccessControl.ACE4_APPEND_DATA, Who.OWNER, -1, ACE.DEFAULT_ADDRESS_MSK));
        ownerAcl.add(new ACE(AceType.ACCESS_DENIED_ACE_TYPE, 0, AccessControl.ACE4_APPEND_DATA, Who.EVERYONE, -1, ACE.DEFAULT_ADDRESS_MSK));
        // this should have no effect
        ownerAcl.add(new ACE(AceType.ACCESS_ALLOWED_ACE_TYPE, 0, AccessControl.ACE4_APPEND_DATA, Who.EVERYONE, -1, ACE.DEFAULT_ADDRESS_MSK));

        // uid2 can't delete, but everyone else can
        ownerAcl.add(new ACE(AceType.ACCESS_DENIED_ACE_TYPE, 0, AccessControl.ACE4_DELETE, Who.USER, uid2, ACE.DEFAULT_ADDRESS_MSK));
        ownerAcl.add(new ACE(AceType.ACCESS_ALLOWED_ACE_TYPE, 0, AccessControl.ACE4_DELETE, Who.EVERYONE, -1, ACE.DEFAULT_ADDRESS_MSK));

        // only the owning group can read acl
        ownerAcl.add(new ACE(AceType.ACCESS_ALLOWED_ACE_TYPE, 0, AccessControl.ACE4_READ_ACL, Who.OWNER_GROUP, -1, ACE.DEFAULT_ADDRESS_MSK));
        ownerAcl.add(new ACE(AceType.ACCESS_DENIED_ACE_TYPE, 0, AccessControl.ACE4_READ_ACL, Who.EVERYONE, -1, ACE.DEFAULT_ADDRESS_MSK));

        // only gid3 can read attributes
        ownerAcl.add(new ACE(AceType.ACCESS_ALLOWED_ACE_TYPE, 0, AccessControl.ACE4_READ_ATTRIBUTES, Who.GROUP, gid3, ACE.DEFAULT_ADDRESS_MSK));
        ownerAcl.add(new ACE(AceType.ACCESS_DENIED_ACE_TYPE, 0, AccessControl.ACE4_READ_ATTRIBUTES, Who.EVERYONE, -1, ACE.DEFAULT_ADDRESS_MSK));

        // basic checks
        assertEquals(AclCheckable.Access.ALLOW, AccessControl.check(subject1, ownerAcl, uid1, gid1, AccessControl.ACE4_APPEND_DATA));
        assertEquals(AclCheckable.Access.DENY, AccessControl.check(subject2, ownerAcl, uid1, gid1, AccessControl.ACE4_APPEND_DATA));

        assertEquals(AclCheckable.Access.DENY, AccessControl.check(subject2, ownerAcl, uid1, gid1, AccessControl.ACE4_DELETE));
        assertEquals(AclCheckable.Access.ALLOW, AccessControl.check(subject1, ownerAcl, uid1, gid1, AccessControl.ACE4_DELETE));

        assertEquals(AclCheckable.Access.ALLOW, AccessControl.check(subject1, ownerAcl, uid1, gid1, AccessControl.ACE4_READ_ACL));
        assertEquals(AclCheckable.Access.ALLOW, AccessControl.check(subject2, ownerAcl, uid1, gid1, AccessControl.ACE4_READ_ACL));
        assertEquals(AclCheckable.Access.DENY, AccessControl.check(subject2, ownerAcl, uid1, gid2, AccessControl.ACE4_READ_ACL));

        assertEquals(AclCheckable.Access.DENY, AccessControl.check(subject1, ownerAcl, uid1, gid1, AccessControl.ACE4_READ_ATTRIBUTES));
        assertEquals(AclCheckable.Access.ALLOW, AccessControl.check(subject2, ownerAcl, uid1, gid1, AccessControl.ACE4_READ_ATTRIBUTES));

        assertEquals(AclCheckable.Access.ALLOW, AccessControl.check(subject1, ownerAcl, uid1, gid1, AccessControl.ACE4_APPEND_DATA));
        assertEquals(AclCheckable.Access.DENY, AccessControl.check(subject2, ownerAcl, uid1, gid1, AccessControl.ACE4_APPEND_DATA));

        assertEquals(AclCheckable.Access.UNDEFINED, AccessControl.check(subject2, ownerAcl, uid1, gid1, AccessControl.ACE4_EXECUTE));

        // check that all bits must pass
        assertEquals(AclCheckable.Access.ALLOW, AccessControl.check(subject1, ownerAcl, uid1, gid1, AccessControl.ACE4_DELETE | AccessControl.ACE4_APPEND_DATA));
        assertEquals(AclCheckable.Access.DENY, AccessControl.check(subject1, ownerAcl, uid1, gid1, AccessControl.ACE4_READ_ATTRIBUTES | AccessControl.ACE4_APPEND_DATA));
    }
}