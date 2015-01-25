package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.apache.commons.codec.binary.Hex;

import javax.crypto.Cipher;
import javax.crypto.SecretKey;
import javax.crypto.spec.IvParameterSpec;
import java.security.Security;
import java.text.MessageFormat;

public class TokenEncrypter {
    private static final String TOKEN_FORMAT = "id: {0}, secret: {1}";

    static {
        Security.addProvider(new org.bouncycastle.jce.provider.BouncyCastleProvider());
    }

    public AuthenticationToken tryParse(SecretKey key, String encrypted) throws SecurityException {
        try {
            Object[] parsed = decryptAndParse(encrypted, key);
            long userId = Long.parseLong((String) parsed[0]);
            String secret = ((String) parsed[1]);
            return new AuthenticationToken(userId, secret);
        } catch (Exception e) {
            throw new SecurityException();
        }
    }

    // FIXME: Ceci n'est pas une signature. This is encrypted data. This should have a signature
    //        however, since CBC mode is vulnerable to padding oracle attacks.
    public String signature(SecretKey secretKey, AuthenticationToken token) {
        try {
            String value = MessageFormat.format(TOKEN_FORMAT, token.getUserId(), token.getSecret());
            byte[] clearText = value.getBytes();
            Cipher cipher = Cipher.getInstance("AES/CBC/PKCS7Padding", "BC");
            // FIXME: There are a plethora of difficult-to-explain attacks enabled by reusing an IV,
            //        however a very simple one is that since there's no other information in the
            //        token than UN/PW, a given pair will always encrypt to the same value. This
            //        allows an attacker to steal the token, maybe from browser history since it's
            //        used in the URL, and then impersonate that user indefinitely.
            //        
            //        Another attack is, if a token can be associated with the UN/PW (probably easy
            //        since that goes over URL parameters as well), they now know the matching
            //        cipher and plaintext, and with a constant IV, multiple associations are very
            //        easily used (much more easily than with a random IV) to reduce the problem
            //        space in searching for the encryption key or other attacks. This known-
            //        plaintext attack was one of the primary tools used for analyzing Enigma
            //        messages in the feature motion picture The Imitation Game, a biographical
            //        thriller about cryptanalyst Alan Turing.
            //
            //        For CBC mode, using a secure random IV is important, and it should be long
            //        enough that it's unlikely to repeat. Due to the Birthday Paradox, this means
            //        you're 50% likely to get a repeat after sqrt(num_values). If a 32-bit integer
            //        is used for example, this happens at 65536 IVs.
            //
            //        Finally, using the key itself as the IV is particularly bad. The IV is not
            //        secret, which often means no effort is made to protect it. In CBC for example:
            //
            //        https://defuse.ca/blog/recovering-cbc-mode-iv-chosen-ciphertext.html
            //
            IvParameterSpec initVector = new IvParameterSpec(secretKey.getEncoded());
            cipher.init(Cipher.ENCRYPT_MODE, secretKey, initVector);
            byte[] encrypted = cipher.doFinal(clearText);
            return Hex.encodeHexString(encrypted);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    private Object[] decryptAndParse(String encrypted, SecretKey key) throws Exception {
        String decrypted = decrypt(encrypted, key);
        return new MessageFormat(TOKEN_FORMAT).parse(decrypted);
    }

    private String decrypt(String encrypted, SecretKey key) throws Exception {
        byte[] bytes = Hex.decodeHex(encrypted.toCharArray());
        Cipher cipher = Cipher.getInstance("AES/CBC/PKCS7Padding", "BC");
        // FIXME: IV reuse. The fact that Initialization Vector's abbreviation is "IV" is not a
        //        coincidence. Actually it is, probably, but pretend it isn't.
        IvParameterSpec initVector = new IvParameterSpec(key.getEncoded());
        cipher.init(Cipher.DECRYPT_MODE, key, initVector);
        return new String(cipher.doFinal(bytes));
    }
}
