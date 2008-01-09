/*
 * Copyright (C) 2001 - 2005 ZJU Online Judge, All Rights Reserved.
 */
package cn.edu.zju.acm.onlinejudge.action;


import java.util.ArrayList;
import java.util.List;

import org.apache.struts.action.ActionForm;
import org.apache.struts.action.ActionForward;
import org.apache.struts.action.ActionMapping;
import org.apache.struts.action.ActionMessages;

import cn.edu.zju.acm.onlinejudge.bean.AbstractContest;
import cn.edu.zju.acm.onlinejudge.bean.Reference;
import cn.edu.zju.acm.onlinejudge.bean.Submission;
import cn.edu.zju.acm.onlinejudge.bean.UserProfile;
import cn.edu.zju.acm.onlinejudge.bean.enumeration.JudgeReply;
import cn.edu.zju.acm.onlinejudge.bean.enumeration.Language;
import cn.edu.zju.acm.onlinejudge.bean.request.ProblemCriteria;
import cn.edu.zju.acm.onlinejudge.bean.request.SubmissionCriteria;
import cn.edu.zju.acm.onlinejudge.bean.request.UserCriteria;
import cn.edu.zju.acm.onlinejudge.form.SubmissionSearchForm;
import cn.edu.zju.acm.onlinejudge.form.UserSearchForm;
import cn.edu.zju.acm.onlinejudge.persistence.ContestPersistence;
import cn.edu.zju.acm.onlinejudge.persistence.ProblemPersistence;
import cn.edu.zju.acm.onlinejudge.security.UserSecurity;
import cn.edu.zju.acm.onlinejudge.util.PersistenceManager;
import cn.edu.zju.acm.onlinejudge.util.StatisticsManager;
import cn.edu.zju.acm.onlinejudge.util.UserManager;
import cn.edu.zju.acm.onlinejudge.util.Utility;

/**
 * <p>
 * ShowUserRoleAction
 * </p>
 * 
 * 
 * @author ZOJDEV
 * @version 2.0
 */
public class ShowUserRoleAction extends BaseAction {
    
	
    /**
     * <p>
     * Default constructor.
     * </p>
     */
    public ShowUserRoleAction() {
    	
    }

    /**
     * ShowRunsAction.
     *
     * @param mapping action mapping
     * @param form action form
     * @param request http sevelet request
     * @param response http sevelet response
     *
     * @return action forward instance
     *
     * @throws Exception any errors happened
     */
    public ActionForward execute(ActionMapping mapping, ActionForm form, ContextAdapter context) throws Exception {
    	
        
        // check admin
        ActionForward forward = checkAdmin(mapping, context);
        if (forward != null) {
            return forward;
        }
        
        long userId = Utility.parseLong(String.valueOf(context.getRequest().getParameter("userId")));
        UserProfile user = UserManager.getInstance().getUserProfile(userId);
        if (user == null) {
            return handleSuccess(mapping, context, "failure");
        }
        
        
        context.getRequest().setAttribute("User", user);        
        context.getRequest().setAttribute("Roles", 
                PersistenceManager.getInstance().getAuthorizationPersistence().getAllRoles());
        context.getRequest().setAttribute("UserRoles", 
                PersistenceManager.getInstance().getAuthorizationPersistence().getUserSecurity(userId).getRoles());

        return handleSuccess(mapping, context, "success");
                  	    	   
    }   
    
    private boolean empty(String value) {
        return value == null || value.trim().length() == 0;
    }

}
    