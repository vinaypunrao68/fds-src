package com.formationds.util.s3.auth;

import com.google.common.collect.Lists;
import org.apache.commons.codec.binary.Hex;
import org.joda.time.DateTime;
import org.joda.time.format.ISODateTimeFormat;
import org.junit.Assert;
import org.junit.Test;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.URL;
import java.util.*;
import java.util.stream.Collectors;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

public class S3SignatureGeneratorV4Test {

    // taken from the example at: http://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-header-based-auth.html
    @Test
    public void referenceWholePayloadTest() throws Exception {
        String shaString = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
        byte[] sha = Hex.decodeHex(shaString.toCharArray());
        String date = "20130524T000000Z";
        DateTime dt = DateTime.parse(date, ISODateTimeFormat.basicDateTimeNoMillis());


        Map<String, Collection<String>> headers = new HashMap<>();
        List<String> headersToSign = Lists.newArrayList("host", "x-amz-content-sha256", "x-amz-date");

        headers.put("Host", Collections.singleton("examplebucket.s3.amazonaws.com"));
        headers.put("x-amz-date", Collections.singleton(date));
        headers.put("x-amz-content-sha256", Collections.singleton(shaString));

        Map<String, Collection<String>> queryParameters = new HashMap<>();
        queryParameters.put("max-keys", Collections.singleton("2"));
        queryParameters.put("prefix", Collections.singleton("J"));

        SignatureRequestData srd = new SignatureRequestData("GET", "/", queryParameters, headers, headersToSign);

        // test canonical request
        String canonicalRequest = srd.buildCanonicalRequest(sha);

        String[] canonicalRequestLines = canonicalRequest.split("\n");
        Assert.assertEquals(9, canonicalRequestLines.length);
        Assert.assertEquals("GET", canonicalRequestLines[0]);
        Assert.assertEquals("/", canonicalRequestLines[1]);
        Assert.assertEquals("max-keys=2&prefix=J", canonicalRequestLines[2]);
        Assert.assertEquals("host:examplebucket.s3.amazonaws.com", canonicalRequestLines[3]);
        Assert.assertEquals("x-amz-content-sha256:e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", canonicalRequestLines[4]);
        Assert.assertEquals("x-amz-date:20130524T000000Z", canonicalRequestLines[5]);
        Assert.assertEquals("", canonicalRequestLines[6]);
        Assert.assertEquals("host;x-amz-content-sha256;x-amz-date", canonicalRequestLines[7]);
        Assert.assertEquals(shaString, canonicalRequestLines[8]);

        String hashString = Hex.encodeHexString(srd.fullCanonicalRequestHash(sha));
        String expected = "df57d21db20da04d7fa30298dd4488ba3a2b47ca3a489c74750e0f1e7df1b9b7";
        Assert.assertEquals(expected, hashString);
    }

    // cf: http://docs.aws.amazon.com/general/latest/gr/signature-v4-test-suite.html
    // TODO: enable this test using the v4 test suite, cannot seem to figure out how
    //       to embed the testsuite.zip file in here
    public void runAwsTestSuite() throws Exception {
        S3SignatureGeneratorV4 sigGen = new S3SignatureGeneratorV4();
        String secretKey = "wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY";
        String credentialScope = "AKIDEXAMPLE/20110909/us-east-1/host/aws4_request";
        String region = "us-east-1";

        String file = "/home/james/Downloads/aws4/aws4_testsuite.zip";

        URL resource = this.getClass().getResource("resources/aws4_testsuite.zip");
        // Assert.assertNotNull("Could not find AWS test suite in resources", resource);
        ZipFile zf = new ZipFile(file);

        // read test data from zip file
        Map<String, Map<String, ZipEntry>> testManifest = new HashMap<>();
        zf.stream().forEachOrdered(entry -> {
            String name = entry.getName();
            if(name.startsWith("aws4_testsuite/")) {
                name = name.replaceFirst("aws4_testsuite/", "");
                String testCase = name.replaceAll("\\.[^.]*$", "");
                testManifest.computeIfAbsent(testCase, k -> new HashMap<>()).put(name, entry);
            }
        });

        // run each test
        for(Map.Entry<String, Map<String, ZipEntry>> testCase : testManifest.entrySet()) {
            String tcName = testCase.getKey();
            Map<String, ZipEntry> testCaseEntries = testCase.getValue();

            // skip cases that are missing important stuff
            if(!testCaseEntries.containsKey(tcName + ".authz") || !testCaseEntries.containsKey(tcName + ".creq") || !testCaseEntries.containsKey(tcName + ".sts"))
                continue;

            System.out.println("Testing " + tcName);

            // get auth header
            V4AuthHeader authHeader =  null;
            try(InputStream authz = zf.getInputStream(testCaseEntries.get(tcName + ".authz"))) {
                BufferedReader rdr = new BufferedReader(new InputStreamReader(authz));
                authHeader = new V4AuthHeader(rdr.readLine());
            }

            // read the expected canonicalRequest
            List<String> expectedCanonicalRequestLines = null;
            try(InputStream creq = zf.getInputStream(testCaseEntries.get(tcName + ".creq"))) {
                expectedCanonicalRequestLines = readLines(creq);
            }

            // read the string to sign
            List<String> expectedStringToSignLines = null;
            try(InputStream sts = zf.getInputStream(testCaseEntries.get(tcName + ".sts"))) {
                expectedStringToSignLines = readLines(sts);
            }

            byte[] contentSha = Hex.decodeHex(expectedCanonicalRequestLines.get(expectedCanonicalRequestLines.size() - 1).toCharArray());

            SignatureRequestData sigData = signatureRequestDataFromStream(zf.getInputStream(testCaseEntries.get(tcName + ".req")), Lists.newArrayList(authHeader.getSignedHeaders()));

            // test canonical request
            String computedCanonicalRequest = sigData.buildCanonicalRequest(contentSha);
            String expectedCanonicalRequest = String.join("\n", expectedCanonicalRequestLines);

            Assert.assertEquals("Canonical request is incorrect for test case" + tcName, expectedCanonicalRequest, computedCanonicalRequest);


            // test string to sign
            String computedStringToSign =  sigGen.createStringToSign(sigData.fullCanonicalRequestHash(contentSha), authHeader.getScope(), sigData.getDate());
            String expectedStringToSign = String.join("\n", expectedStringToSignLines);
            Assert.assertEquals("String to sign is incorrect for test case " + tcName, expectedStringToSign, computedStringToSign);

            // test hash
            byte[] expectedSigHash = Hex.decodeHex(authHeader.getSignature().toCharArray());
            byte[] sigHash = sigGen.fullContentSignature(secretKey, sigData, authHeader.getScope(), contentSha);

            Assert.assertArrayEquals(expectedSigHash, sigHash);
        }
    }

    private List<String> readLines(InputStream creq) {
        List<String> expectedCanonicalRequestLines;BufferedReader rdr = new BufferedReader(new InputStreamReader(creq));
        expectedCanonicalRequestLines = rdr.lines().collect(Collectors.toList());
        return expectedCanonicalRequestLines;
    }

    private List<String> extractSignedHeaders(String authHeader) {
        V4AuthHeader header = new V4AuthHeader(authHeader);
        return Lists.newArrayList(header.getSignedHeaders());
    }

    private String plusToSpace(String input) {
        return input.replace('+', ' ');
    }

    private SignatureRequestData signatureRequestDataFromStream(InputStream stream, List<String> signedHeadersList) throws Exception {
        BufferedReader rdr = new BufferedReader(new InputStreamReader(stream));
        String requestLine = rdr.readLine();
        String[] reqLineParts = requestLine.split("\\s+");
        String method = reqLineParts[0];
        String fullPath = reqLineParts[1];

        String[] pathParts = fullPath.split("\\?", 2);
        String path = pathParts[0];
        Map<String, Collection<String>> queryParameters = new HashMap<>();
        if(pathParts.length > 1) {
            String[] queryParts = pathParts[1].split("&");
            for(String queryPart : queryParts) {
                String[] kvp = queryPart.split("=", 2);

                if(kvp.length > 1)
                    queryParameters.computeIfAbsent(plusToSpace(kvp[0]), k -> new ArrayList<>()).add(plusToSpace(kvp[1]));
                else
                    queryParameters.computeIfAbsent(plusToSpace(kvp[0]), k -> new ArrayList<>()).add("");
            }
        }

        Map<String, Collection<String>> headers = new TreeMap<>(String.CASE_INSENSITIVE_ORDER);
        while(true) {
            String line = rdr.readLine();
            if(line == null || line.isEmpty())
                break;

            String[] headerParts = line.split(":\\s*", 2);
            headers.computeIfAbsent(headerParts[0], k -> new ArrayList<>()).add(headerParts[1]);
        }

        return new SignatureRequestData(method, path, queryParameters, headers, signedHeadersList);
    }
}