/*
 * Copyright (C) 2001 - 2005 ZJU Online Judge, All Rights Reserved.
 */
package cn.edu.zju.acm.onlinejudge.bean.enumeration;

import java.util.ArrayList;
import java.util.List;

/**
 * <p>
 * This class represents the Judge Reply.
 * </p>
 * 
 * @author ZOJDEV
 * @version 2.0
 */
public class JudgeReply {

    private static List<JudgeReply> replies = new ArrayList<JudgeReply>();    

    public final static JudgeReply QUEUING = new JudgeReply(0, "Queuing", "Queuing", null, false);

    public final static JudgeReply COMPILING = new JudgeReply(1, "Compiling", "Compiling", null, false);

    public final static JudgeReply RUNNING = new JudgeReply(2, "Running", "Running", null, false);

    public final static JudgeReply RUNTIME_ERROR = new JudgeReply(3, "Runtime Error", "Runtime Error", null, false);

    public final static JudgeReply WRONG_ANSWER = new JudgeReply(4, "Wrong Answer", "Wrong Answer", null, false);

    public final static JudgeReply ACCEPTED = new JudgeReply(5, "Accepted", "Accepted", null, false);

    public final static JudgeReply TIME_LIMIT_EXCEEDED = new JudgeReply(6, "Time Limit Exceeded",
            "Time Limit Exceeded", null, false);

    public final static JudgeReply MEMORY_LIMIT_EXCEEDED = new JudgeReply(7, "Memory Limit Exceeded",
            "Memory Limit Exceeded", null, false);

    public final static JudgeReply OUT_OF_CONTEST_TIME = new JudgeReply(8, "Out of Contest Time",
            "Out of Contest Time", null, false);

    public final static JudgeReply RESTRICTED_FUNCTION = new JudgeReply(9, "Restricted Function",
            "Restricted Function", null, false);

    public final static JudgeReply OUTPUT_LIMIT_EXCEEDED = new JudgeReply(10, "Output Limit Exceeded",
            "Output Limit Exceeded", null, false);

    public final static JudgeReply COMPILATION_ERROR = new JudgeReply(12, "Compilation Error", "Compilation Error",
            null, false);

    public final static JudgeReply PRESENTATION_ERROR = new JudgeReply(13, "Presentation Error", "Presentation Error",
            null, false);

    public final static JudgeReply JUDGE_INTERNAL_ERROR = new JudgeReply(14, "Judge Internal Error",
            "Judge Internal Error", null, false);

    public final static JudgeReply FLOATING_POINT_ERROR = new JudgeReply(15, "Floating Point Error",
            "Floating Point Error", null, false);

    public final static JudgeReply SEGMENTATION_FAULT = new JudgeReply(16, "Segmentation Fault", "Segmentation Fault",
            null, false);

    public final static JudgeReply JUDGING = new JudgeReply(19, "Judging", "Judging", null, false);

    public final static JudgeReply SUBMISSION_LIMIT_EXCEEDED = new JudgeReply(20, "Submission Limit Exceeded",
            "Submission Limit Exceeded", null, false);
    public final static JudgeReply READY = new JudgeReply(100, "Ready", "Ready", null, false);
    public final static JudgeReply UNSUPPORTED_SOURCE_FILE_TYPE = new JudgeReply(101, "Unsupported Source File Type",
            "Unsupported Source File Type", null, false);
    public final static JudgeReply NO_SUCH_PROBLEM = new JudgeReply(102, "No Such Problem",
            "No Such Problem", null, false);
    public final static JudgeReply INVALID_TESTCASE = new JudgeReply(103, "Invalid Testcase",
            "Invalid Testcase", null, false);
    public final static JudgeReply INVALID_TIME_LIMIT = new JudgeReply(104, "Invalid Time Limit",
            "Invalid Time Limit", null, false);
    public final static JudgeReply INVALID_MEMORY_LIMIT = new JudgeReply(105, "Invalid Time Limit",
            "Invalid Time Limit", null, false);
    public final static JudgeReply INVALID_OUTPUT_LIMIT = new JudgeReply(106, "Invalid Output Limit",
            "Invalid Output Limit", null, false);
    public final static JudgeReply INVALID_DATA_SIZE = new JudgeReply(107, "Invalid Data Size",
            "Invalid Data Size", null, false);
    public final static JudgeReply INVALID_DATA = new JudgeReply(108, "Invalid Data",
            "Invalid Data", null, false);
    public final static JudgeReply INVALID_SOURCE_FILE_TYPE = new JudgeReply(109, "Invalid Source File Type",
            "Invalid Source File Type", null, false);
    /**
     * <p>
     * Represents the id of JudgeReply.
     * </p>
     */
    private long id;

    /**
     * <p>
     * Represents the name of JudgeReply.
     * </p>
     */
    private String name;

    /**
     * <p>
     * Represents the description of JudgeReply.
     * </p>
     */
    private String description;

    /**
     * <p>
     * Represents whether it is committed.
     * </p>
     */
    private boolean committed;

    /**
     * <p>
     * Represents the style of JudgeReply.
     * </p>
     */
    private String style;
    
    /**
     * <p>
     * Create a new instance of JudgeReply.
     * </p>
     * 
     * @param id
     *            the id of JudgeReply.
     * @param name
     *            the name of JudgeReply.
     * @param description
     *            the description of JudgeReply.
     * @param committed
     *            whether it is committed.
     * @param style
     *            wthe style
     * 
     * @throws NullPointerException
     *             if any argument is null.
     * @throws IllegalArgumentException
     *             if the name is an empty string.
     */
    public JudgeReply(long id, String name, String description, String style, boolean committed) {
        if (name == null) {
            throw new NullPointerException("name should not be null.");
        }
        if (description == null) {
            throw new NullPointerException("description should not be null.");
        }
        if (name.trim().length() == 0) {
            throw new IllegalArgumentException("name should not be empty string.");
        }
        this.id = id;
        this.name = name;
        this.description = description;
        this.committed = committed;
        this.style = style;
        replies.add(this);
    }

    /**
     * <p>
     * Get the id of the JudgeReply.
     * </p>
     * 
     * @return id the id of the JudgeReply.
     */
    public long getId() {
        return this.id;
    }

    /**
     * <p>
     * Get the name of the JudgeReply.
     * </p>
     * 
     * @return id the name of the JudgeReply.
     */
    public String getName() {
        return this.name;
    }

    /**
     * <p>
     * Get the description of the JudgeReply.
     * </p>
     * 
     * @return id the description of the JudgeReply.
     */
    public String getDescription() {
        return this.description;
    }

    /**
     * <p>
     * whether the JudgeReply is committed.
     * </p>
     * 
     * @return true if the JudgeReply is committed.
     */
    public boolean isCommitted() {
        return this.committed;
    }

    /**
     * <p>
     * Get the style of the JudgeReply.
     * </p>
     * 
     * @return id the style of the JudgeReply.
     */
    public String getStyle() {
        return this.style;
    }

    /**
     * <p>
     * Return the name.
     * 
     * @return the name
     */
    public String toString() {
        return this.name;
    }

    /**
     * <p>
     * Compares to the object.
     * </p>
     * 
     * @return true if and only if the ids of the two are the same.
     * 
     * @param object
     *            the object to compare.
     */
    public boolean equals(Object object) {
        return object instanceof JudgeReply && this.id == ((JudgeReply) object).id;
    }

    /**
     * <p>
     * get the hashCode of this instance.
     * </p>
     * 
     * @return the hash code of this instance.
     */
    public int hashCode() {
        return new Long(this.id).hashCode();
    }

    /**
     * <p>
     * Get the JudgeReply Type represented by id.
     * </p>
     * 
     * @return the JudgeReply Type represented by id, or null if none is found.
     * 
     * @param id
     *            the id of JudgeReply
     */
    public static JudgeReply findById(long id) {
        for (JudgeReply reply : replies) {
            if (reply.getId() == id) {
                return reply;
            }
        }
        return null;
    }

    /**
     * <p>
     * Get the JudgeReply Type represented by name.
     * </p>
     * 
     * @return the JudgeReply Type represented by name, or null if none matchs.
     * 
     * @param name
     *            the name of JudgeReply.
     */
    public static JudgeReply findByName(String name) {
        for (JudgeReply reply : replies) {
            if (reply.getName().equals(name)) {
                return reply;
            }
        }
        return null;

    }

}
