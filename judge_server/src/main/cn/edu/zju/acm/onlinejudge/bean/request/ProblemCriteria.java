
package cn.edu.zju.acm.onlinejudge.bean.request;

/**
 * <p></p>
 * @poseidon-object-id [I81be8am1050b02b322mm507d]
 */
public class ProblemCriteria {

/**
 * <p>Represents ...</p>
 * @poseidon-object-id [I81be8am1050b02b322mm506a]
 */
    private Long contestId;

/**
 * <p>Represents ...</p>
 * @poseidon-object-id [I81be8am1050b02b322mm5059]
 */
    private String title;

/**
 * <p>Represents ...</p>
 * @poseidon-object-id [I81be8am1050b02b322mm5048]
 */
    private String author;

/**
 * <p>Represents ...</p>
 * @poseidon-object-id [I81be8am1050b02b322mm5037]
 */
    private Long userProfileId;

/**
 * <p>Represents ...</p>
 * @poseidon-object-id [I81be8am1050b02b322mm5026]
 */
    private Boolean solved;

/**
 * <p>Represents ...</p>
 * @poseidon-object-id [I81be8am1050b02b322mm5015]
 */
    private Boolean submitted;

/**
 * <p>Represents ...</p>
 * @poseidon-object-id [I81be8am1050b02b322mm5004]
 */
    private String content;
    
    /**
     * 
     * @return
     */
    public Long getContestId() {
    	return contestId;
    }
    public void setContestId(Long contestId) {
    	this.contestId = contestId;    	
    }
    
    public Long getUserProfileId() {
    	return userProfileId;
    }
    public void setUserProfileId(Long userProfileId) {
    	this.userProfileId = userProfileId;    	
    }
    
    public int hashCode() {
    	int hash = 0;
    	hash = cal(author, hash);
    	hash = cal(content, hash);
    	hash = cal(contestId, hash);
    	hash = cal(solved, hash);
    	hash = cal(submitted, hash);
    	hash = cal(title, hash);
    	hash = cal(userProfileId, hash);
    	return hash;    	
    }
    private int cal(Object obj, int hash) {
    	hash = (hash >>> 3);
    	if (obj == null) {
    		return hash ^ 1234567891;
    	} else {
    		return hash ^ obj.hashCode();
    	}
    }
    public boolean equals(Object obj) {
    	if (!(obj instanceof ProblemCriteria)) {
    		return false;
    	}
    	ProblemCriteria that = (ProblemCriteria) obj;
    	if (!equals(this.author, that.author)) {
    		return false;
    	}
    	if (!equals(this.content, that.content)) {
    		return false;
    	}
    	if (!equals(this.contestId, that.contestId)) {
    		return false;
    	}
    	if (!equals(this.solved, that.solved)) {
    		return false;
    	}
    	if (!equals(this.submitted, that.submitted)) {
    		return false;
    	}
    	if (!equals(this.title, that.title)) {
    		return false;
    	}
    	if (!equals(this.userProfileId, that.userProfileId)) {
    		return false;
    	}
    	return true;
    	
    }
    private boolean equals(Object o1, Object o2) {
    	if (o1 == null) {
    		return o2 == null;
    	} 
    	return o1.equals(o2);
    }
 }
