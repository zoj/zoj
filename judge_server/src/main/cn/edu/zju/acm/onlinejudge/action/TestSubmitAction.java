/*
 * Copyright 2007 Zhang, Zheng <oldbig@gmail.com>
 * 
 * This file is part of ZOJ.
 * 
 * ZOJ is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either revision 3 of the License, or (at your option) any later revision.
 * 
 * ZOJ is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with ZOJ. if not, see
 * <http://www.gnu.org/licenses/>.
 */

package cn.edu.zju.acm.onlinejudge.action;

import java.util.Date;
import java.util.List;
import java.util.Random;

import org.apache.struts.action.ActionForm;
import org.apache.struts.action.ActionForward;
import org.apache.struts.action.ActionMapping;
import org.apache.struts.action.ActionMessage;
import org.apache.struts.action.ActionMessages;

import cn.edu.zju.acm.onlinejudge.bean.AbstractContest;
import cn.edu.zju.acm.onlinejudge.bean.Problem;
import cn.edu.zju.acm.onlinejudge.bean.Reference;
import cn.edu.zju.acm.onlinejudge.bean.Submission;
import cn.edu.zju.acm.onlinejudge.bean.UserProfile;
import cn.edu.zju.acm.onlinejudge.bean.enumeration.JudgeReply;
import cn.edu.zju.acm.onlinejudge.bean.enumeration.Language;
import cn.edu.zju.acm.onlinejudge.bean.enumeration.ReferenceType;
import cn.edu.zju.acm.onlinejudge.judgeservice.JudgeService;
import cn.edu.zju.acm.onlinejudge.judgeservice.Priority;
import cn.edu.zju.acm.onlinejudge.persistence.SubmissionPersistence;
import cn.edu.zju.acm.onlinejudge.util.ConfigManager;
import cn.edu.zju.acm.onlinejudge.util.PersistenceManager;
import cn.edu.zju.acm.onlinejudge.util.Utility;
import cn.edu.zju.acm.onlinejudge.util.cache.Cache;

/**
 * <p>
 * SubmitAction
 * </p>
 * 
 * 
 * @author Zhang, Zheng
 * @version 2.0
 */
public class TestSubmitAction extends BaseAction {

	private static Cache<Long> submitCache = null;
	
	static {
		long interval = Utility.parseLong(ConfigManager.getValue("submit_interval"));
		if (interval > 0) {
			submitCache = new Cache<Long>(interval, Integer.MAX_VALUE);
		}
	}
	
    /**
     * <p>
     * Default constructor.
     * </p>
     */
    public TestSubmitAction() {
    // empty
    }

    /**
     * SubmitAction.
     * 
     * @param mapping
     *            action mapping
     * @param form
     *            action form
     * @param request
     *            http servlet request
     * @param response
     *            http servlet response
     * 
     * @return action forward instance
     * 
     * @throws Exception
     *             any errors happened
     */
    @Override
    public ActionForward execute(ActionMapping mapping, ActionForm form, ContextAdapter context) throws Exception {

        if (!this.isLogin(context, true)) {
            return this.handleSuccess(mapping, context, "login");
        }

        boolean isProblemset = context.getRequest().getRequestURI().endsWith("submit.do");

        ActionForward forward = this.checkProblemParticipatePermission(mapping, context, isProblemset);
        if (forward != null) {
            return forward;
        }

        AbstractContest contest = context.getContest();
        Problem problem = context.getProblem();

        long languageId = Utility.parseLong(context.getRequest().getParameter("languageId"));
        Language language = PersistenceManager.getInstance().getLanguagePersistence().getLanguage(languageId);
        if (language == null) {
            return this.handleSuccess(mapping, context, "submit");
        }
        String source = context.getRequest().getParameter("source");
        if (source == null || source.length() == 0) {
            return this.handleSuccess(mapping, context, "submit");
        }
        
        List refrance = PersistenceManager.getInstance().getReferencePersistence().getProblemReferences(problem.getId(), ReferenceType.HEADER);
        if(refrance.size()!=0) {
        	Reference r = (Reference)refrance.get(0);
        	String percode = new String(r.getContent());
        	source=percode+"\n"+source;
        }
        
        UserProfile user = context.getUserProfile();
        if (submitCache != null && submitCache.contains(user.getId())) {
        	 
        	ActionMessages messages = new ActionMessages();       
            messages.add("message", new ActionMessage("onlinejudge.submit.interval"));
            this.saveErrors(context.getRequest(), messages);
            
            context.setAttribute("source", source);
            
        	return handleSuccess(mapping, context, "submit");
        }
        
        if (contest.isCheckIp()) {
            forward = this.checkLastLoginIP(mapping, context, isProblemset);
            if (forward != null) {
                return forward;
            }
        }
        Submission submission = new Submission();
        submission.setContestId(contest.getId());
        submission.setLanguage(language);
        submission.setProblemId(problem.getId());
        submission.setUserProfileId(user.getId());
        submission.setContent(source);
        submission.setMemoryConsumption(0);
        submission.setTimeConsumption(0);
        submission.setSubmitDate(new Date());
        SubmissionPersistence submissionPersistence = PersistenceManager.getInstance().getSubmissionPersistence();

        if (contest.getEndTime() != null && new Date().after(contest.getEndTime())) {
            submission.setJudgeReply(JudgeReply.OUT_OF_CONTEST_TIME);
            submissionPersistence.createSubmission(submission, user.getId());
        } else if (source.getBytes().length > problem.getLimit().getSubmissionLimit() * 1024) {
            submission.setContent(source.substring(0, problem.getLimit().getSubmissionLimit() * 1024));
            submission.setJudgeReply(JudgeReply.SUBMISSION_LIMIT_EXCEEDED);
            submissionPersistence.createSubmission(submission, user.getId());
        } else {
        	Random ran=new Random();
            submission.setJudgeReply(ran.nextInt()%2==0 ? JudgeReply.WRONG_ANSWER : JudgeReply.ACCEPTED);
            submissionPersistence.createSubmission(submission, user.getId());
            JudgeService.getInstance().judge(submission, Priority.NORMAL);
        }
        context.setAttribute("contestOrder", submission.getContestOrder());
        if (submitCache != null) {
        	submitCache.put(user.getId(), user.getId());
        }
        return this.handleSuccess(mapping, context, "success");

    }
}
