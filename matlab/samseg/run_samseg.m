function run_samseg(imageFileName1,savePath,nThreadsStr,UseGPUStr,imageFileName2,imageFileName3,imageFileName4,imageFileName5,imageFileName6)
% This function is a wrapper for running the samseg matlab
% scripts. This wrapper can be compiled (meaning that all the
% inputs are strings).
%
% modified clarsen 2017/04/23
%
% $Id: run_samseg.m,v 1.1 2017/01/26 00:22:47 greve Exp $
%

fprintf('Matlab version %s\n',version);

nThreads = sscanf(nThreadsStr,'%d');
useGPU = sscanf(UseGPUStr,'%d'); %#ok

fprintf('input file1 %s\n',imageFileName1);
fprintf('input file2 %s\n',imageFileName2);
fprintf('input file3 %s\n',imageFileName3);
fprintf('input file4 %s\n',imageFileName4);
fprintf('input file5 %s\n',imageFileName5);
fprintf('input file6 %s\n',imageFileName6);
fprintf('output path %s\n',savePath);
fprintf('nThreads = %d\n',nThreads);

if(strcmp(imageFileName2,'none')), imageFileName2 = ''; end
if(strcmp(imageFileName3,'none')), imageFileName3 = ''; end
if(strcmp(imageFileName4,'none')), imageFileName4 = ''; end
if(strcmp(imageFileName5,'none')), imageFileName5 = ''; end
if(strcmp(imageFileName6,'none')), imageFileName6 = ''; end

imageFileName = imageFileName1;

% set SAMSEG_DATA_DIR as an environment variable, eg,
%  setenv SAMSEG_DATA_DIR /autofs/cluster/koen/koen/GEMSapplications/wholeBrain
AvgDataDir = getenv('SAMSEG_DATA_DIR');

meshCollectionFileName = sprintf('%s/CurrentMeshCollection30New.txt.gz',AvgDataDir);
% This is bascially an LUT
compressionLookupTableFileName = sprintf('%s/namedCompressionLookupTable.txt',AvgDataDir);

samsegStartTime = tic;

fprintf('running kvlClear\n');
kvlClear; % Clear all the wrapped C++ stuff
close all;

% [clarsen] refactored functions + elastix init of affine atlas to image transform
% this variable and contents were previously hardccoded into samsegment
transformedTemplateFileName = sprintf('%s/mni305_masked_autoCropped_coregistered.mgz',savePath);

templateCoregInitMode = getenv('SAMSEG_COREG_INIT');
spmAffine = false;

switch templateCoregInitMode
    case 'elastix'
        fprintf('Initializing atlas transform using elastix\n');
        
        % for the case where we are in elastix mode
        % requires the full path to the desired template (intensity) image + elastix rigid + affine parameter files
        % TODO: don't env stuff like this... set it in function input instead
        elastixTemplate = getenv('ELASTIX_TEMPLATE_NAME');
        elastixRigid = getenv('ELASTIX_RIGID_PARAMS');
        elastixAffine = getenv('ELASTIX_AFFINE_PARAMS');
        mniTemplateFileName = getenv('ELASTIX_EXTRA_TEMPLATE_NAME');
        
        % note: this variable is really the one which contains the transformed template path in samsegment - hardcoded.
        % so I refactored out from the samseg script to allow override
        [transformedTemplateFileName, transform, extraTransform] = samsegCoregisterElastix(...
            elastixTemplate, ...
            imageFileName1, ...
            elastixRigid, ...
            elastixAffine, ...
            savePath, ...
            mniTemplateFileName);
        if isempty(transformedTemplateFileName)
            disp('elastix failure, exiting')
            return
        end
        
        % this part is excessive - and ONLY this way for testing samseg and to obtain a template using kvl methods fast
        % (the mgz from above is missing some header stuff, i.e., normalization of the transform
        % rewrite later, if things work
        %

        % preserved for debugging       
        [kvlTemplate, ~] = kvlReadImage(elastixTemplate);
        kvlTransform = kvlCreateTransform(transform);
        % write with kvl to make sure things are OK
        transformedTemplateFileName =  [savePath, '/template_coregistered_kvl.mgz'];
        kvlWriteImage(kvlTemplate, transformedTemplateFileName, kvlTransform);
        

        fprintf('done with elastix\n');
        disp(extraTransform)
        % samsegment depends on the cropped template (differences in transform due to translation), so trying 
        % the estimated world-to-world using the extra template (mni305)
        [kvlTemplate2, ~] = kvlReadImage(mniTemplateFileName);
        kvlTransform2 = kvlCreateTransform(extraTransform);
        % write with kvl to make sure things are OK
        transformedTemplateFileName =  [savePath, '/template_coregistered_kvl_with_mni.mgz'];
        kvlWriteImage(kvlTemplate2, transformedTemplateFileName, kvlTransform2); 
        
    case 'spm'
        fprintf('running registerToAtlas\n');
        % Switch on if you want to initialize the registration by matching
        % (translation) the centers  of gravity
        initializeUsingCenterOfGravityAlignment = 0;
        templateFileName = sprintf('%s/mni305_masked_autoCropped.mgz',AvgDataDir);
        
        transformedTemplateFileName = samseg_registerToAtlas(...
            imageFileName,...
            templateFileName,...
            meshCollectionFileName,...
            compressionLookupTableFileName,...
            initializeUsingCenterOfGravityAlignment,...
            savePath, ...
            showFigs);
        
        spmAffine = true;
        
    case 'samseg'
        fprintf('entering registerAtlas\n');
        K = 1e-7; % Mesh stiffness -- compared to normal models, the entropy cost function is normalized
        % (i.e., measures an average *per voxel*), so that this needs to be scaled down by the
        % number of voxels that are covered
        showFigures = true;
        initializeUsingCenterOfGravityAlignment = false;
        templateFileName = sprintf('%s/mni305_masked_autoCropped.mgz',AvgDataDir);
        useInitialTransformForMeshRestPosition = true;
        
        transformedTemplateFileName = samseg_registerAtlas(...
            imageFileName,...
            templateFileName,...
            meshCollectionFileName,...
            compressionLookupTableFileName,...
            initializeUsingCenterOfGravityAlignment,...
            K, ...
            useInitialTransformForMeshRestPosition, ...
            savePath, ...
            showFigures);
end

fprintf('running samsegment \n');
%subject with different contrasts you can feed those in as well. Make sure
%though that the scans are coregistered
%imageFileNames{2} = '...'; %If you have more scans of the same
imageFileNames = cell(0,0);
imageFileNames{1} =  imageFileName1;
if(strlen(imageFileName2)>0)
    imageFileNames{2} =  imageFileName2;
end
if(strlen(imageFileName3)>0)
    imageFileNames{3} =  imageFileName3;
end
if(strlen(imageFileName4)>0)
    imageFileNames{4} =  imageFileName4;
end
if(strlen(imageFileName5)>0)
    imageFileNames{5} =  imageFileName5;
end
if(strlen(imageFileName6)>0)
    imageFileNames{6} =  imageFileName6;
end


downSamplingFactor = 1;  % Use 1 for no downsampling
maxNuberOfIterationPerMultiResolutionLevel(1) = 5; % default 5
maxNuberOfIterationPerMultiResolutionLevel(2) = 20; % default 20

maximumNumberOfDeformationIterations = 500;
maximalDeformationStopCriterion = 0.001; % Measured in pixels
lineSearchMaximalDeformationIntervalStopCriterion = maximalDeformationStopCriterion; % Idem

meshSmoothingSigmas = [ 2.0 0 ]'; % UsemeshSmoothingSigmas = [ 0 ]' if you don't want to use multi-resolution
relativeCostDecreaseStopCriterion = 1e-6;
maximalDeformationAppliedStopCriterion = 0.0;

BFGSMaximumMemoryLength = 12;
K = 0.1; % Stiffness of the mesh

if spmAffine
    brainMaskingSmoothingSigma = 2; % sqrt of the variance of a Gaussian blurring kernel
    brainMaskingThreshold = 0.01;
else
    brainMaskingSmoothingSigma = 5; % 2; % sqrt of the variance of a Gaussian blurring kernel
    brainMaskingThreshold = 0.01;
end

showImages = false;
debug = true;

% run it
samsegment( ...
    imageFileName,...
    imageFileNames, ...
    savePath, ...
    transformedTemplateFileName, ...
    meshCollectionFileName, ...
    compressionLookupTableFileName, ...
    maxNuberOfIterationPerMultiResolutionLevel, ...
    downSamplingFactor, ...
    maximumNumberOfDeformationIterations, ...
    maximalDeformationStopCriterion, ...
    lineSearchMaximalDeformationIntervalStopCriterion, ...
    meshSmoothingSigmas, ...
    relativeCostDecreaseStopCriterion, ...
    maximalDeformationAppliedStopCriterion, ...
    BFGSMaximumMemoryLength, ...
    K, ...
    brainMaskingSmoothingSigma, ...
    brainMaskingThreshold, ...
    nThreads, ...
    showImages, ...
    debug)


fprintf('#@# samseg done %6.4f min\n',toc( samsegStartTime )/60);

if isdeployed
    exit;
end