function [movingCoregistered, transform, extraTransform] = samsegCoregisterElastix(moving, fixed, rigid, affine, outDir, extraTemplatePath)
%
% This function requires elastix to be in the PATH, and libANNlib.so to be in LD_LIBRARY_PATH
%   The function respects original file formats, but does a necessary intermediate transform
%   to and from nifti, in order to be elastix compatible
%
% Input:
%   moving: the image to be transformed (typically the template)
%   fixed: the target image (typically the image to be segmented)
%   rigid: a txt file with elastix compatible settings for a rigid transform
%   affine: a txt file with elastix compatible settings for an affine transform
%   out: the out directory where all files will be written to
%
% Output:
%   movingOut: the full path to the coregistered, moving image

%
% first, we need the images to be in nifti format, else elastix will reject
%

if nargin < 5
    outDir = '.';
end

movingCoregistered = '';

% ensure out dir exists...
outDirTmp = [outDir, '/tmpElastix'];
unix(['mkdir -p ', outDirTmp]);

% convert images
[~, fixedName, fixedExt] = fileparts(fixed);
if ~strcmp(fixedExt,'.nii')
    [status, result] = unix(['mri_convert ', fixed, ' ', outDirTmp, '/', fixedName, '.nii']);
    if status ~= 0
        disp(result)
        return
    end
    fixed = [outDirTmp, '/', fixedName, '.nii'];
    
end
[~, movingName, movingExt] = fileparts(moving);
if ~strcmp(movingExt, '.nii')
    [status, result] = unix(['mri_convert ', moving, ' ', outDirTmp, '/', movingName, '.nii']);
    if status ~= 0
        disp(result)
        return
    end
    moving = [outDirTmp, '/', movingName, '.nii'];
end

%
% now do the two-part elastix registration using first the rigid parameters, then the affine
%

[status, result] = unix(['elastix -f ', fixed ' -m ', moving, ' -p ', rigid, ' -p ', affine, ' -out ', outDirTmp]);
if status ~= 0
    disp(result)
    return
end

% elastix is a little bitch that does not allow one to output the result image with the transform but non-resampled data
% instead, they enforce resampling and use the original header!
% so we need to do this manually! Yay for bad engineering
rigidS = readElastixParameters([outDirTmp, '/TransformParameters.0.txt']);
affineS = readElastixParameters([outDirTmp, '/TransformParameters.1.txt']);

%
% Define the rigid transformation
%

% I assume the order is correct...
yaw = rigidS.TransformParameters(1);
pitch = rigidS.TransformParameters(2);
roll = rigidS.TransformParameters(3);

rt = rigidS.TransformParameters(4:6)';
rc = rigidS.CenterOfRotationPoint';
%  columns stored along the rows, therefore transposed
Rx = [1 0 0; 0 cos(yaw) sin(yaw); 0 -sin(yaw) cos(yaw)]';
Ry = [cos(pitch) 0 -sin(pitch); 0 1 0; sin(pitch) 0 cos(pitch)]';
Rz = [cos(roll) sin(roll) 0; -sin(roll) cos(roll) 0; 0 0 1]';
R = Rz * Ry * Rx;

Tr = [eye(3) (rc+ rt); 0 0 0 1] * [R [0 0 0]'; 0 0 0 1] * [eye(3) (-rc); 0 0 0 1];

%
% Define the affine transformation
%
ac = affineS.CenterOfRotationPoint';
A = reshape(affineS.TransformParameters(1:9), [3 3])';  
at = affineS.TransformParameters(10:12)';

Ta =  [eye(3) (ac+ at); 0 0 0 1] * [A [0 0 0]'; 0 0 0 1] * [eye(3) (-ac); 0 0 0 1];

% respect the direction cosines (I guess)
% this dude seems to be derived from the axis orientation in mni space, e.g., in the mni305 template
% it should never change - unless we change the template image
Tc = [ -1 0 0 0; 0 -1 0 0; 0 0 1 0; 0 0 0 1 ];

%
% Final transform
% imgToWorldCoRegged = X = inv(T) * imgToWorld = T \ imgToWorld
%

template = MRIread(moving);
transform = (Tc * Ta * Tr * Tc) \ template.vox2ras0;

%
% save = make sure derived info is computed
%

%
% now back to original image format
%

newfile.vox2ras0 = transform;
newfile.vol = template.vol;
MRIwrite(newfile, [outDirTmp, '/', movingName, '_coregistered_non_sampled', fixedExt]);

% checking what happens to mni305 masked and cropped...
if ~isempty(extraTemplatePath)
mniTemplate = MRIread(extraTemplatePath);
extraTransform = (Tc * Ta * Tr * Tc) \ mniTemplate.vox2ras0;

% totally a joke on the outputname
movingOut = [outDir '/', movingName, '_coregistered', fixedExt];
[status, result] = unix(['mri_convert ', outDirTmp, '/result.1.nii ', movingOut]);
if status ~= 0
    disp(result)
    return
end

movingCoregistered = movingOut;

end
